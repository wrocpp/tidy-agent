#include "DefaultThenAssignCheck.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <optional>

using namespace clang::ast_matchers;

namespace clang::tidy::wrocpp {

namespace {

// docs/checks/default-then-assign.md: two assignments already say the value
// twice; three mutator calls, because two is common for a pair of sentinels.
constexpr unsigned default_min_assignments = 2;
constexpr unsigned default_min_mutator_calls = 3;
constexpr StringRef default_mutator_names = "push_back;emplace_back;insert;emplace;add";

// True when `expr` refers to `var` anywhere inside it.
bool mentions(const Stmt* stmt, const VarDecl* var) {
    struct Finder : RecursiveASTVisitor<Finder> {
        const VarDecl* target;
        bool found = false;
        bool VisitDeclRefExpr(DeclRefExpr* ref) {
            if (ref->getDecl() == target) found = true;
            return !found;
        }
    } finder{.target = var};
    finder.TraverseStmt(const_cast<Stmt*>(stmt));
    return finder.found;
}

// A statement in a block, without the ExprWithCleanups and implicit nodes
// Clang may wrap around a full-expression.
const Stmt* strip(const Stmt* stmt) {
    const auto* expr = dyn_cast<Expr>(stmt);
    return expr ? expr->IgnoreImplicit() : stmt;
}

// The range to delete for a statement: the whole line when the statement is
// alone on it (indentation and newline included), otherwise just the text.
CharSourceRange removal_range(const Stmt* stmt, SourceLocation after_semi, const SourceManager& sm) {
    SourceLocation begin = stmt->getBeginLoc();
    SourceLocation end = after_semi;
    const char* start = sm.getCharacterData(begin);
    const char* line_start = start;
    const char* buffer_start = sm.getBufferData(sm.getFileID(begin)).data();
    while (line_start > buffer_start && (line_start[-1] == ' ' || line_start[-1] == '\t')) --line_start;
    const char* stop = sm.getCharacterData(after_semi);
    const char* line_end = stop;
    while (*line_end == ' ' || *line_end == '\t') ++line_end;
    if ((line_start == buffer_start || line_start[-1] == '\n') && (*line_end == '\n' || *line_end == '\r')) {
        begin = begin.getLocWithOffset(-(start - line_start));
        end = after_semi.getLocWithOffset((line_end - stop) + (*line_end == '\r' ? 2 : 1));
    }
    return CharSourceRange::getCharRange(begin, end);
}

const DeclRefExpr* as_ref_to(const Expr* expr, const VarDecl* var) {
    const auto* ref = dyn_cast_or_null<DeclRefExpr>(expr ? expr->IgnoreParenImpCasts() : nullptr);
    return ref && ref->getDecl() == var ? ref : nullptr;
}

struct FieldAssignment {
    const Stmt* stmt;
    const FieldDecl* field;
    const Expr* value;
};

// `var.field = value;` where value does not mention var.
std::optional<FieldAssignment> field_assignment(const Stmt* original, const VarDecl* var) {
    const Stmt* stmt = strip(original);
    const Expr* lhs = nullptr;
    const Expr* rhs = nullptr;
    if (const auto* op = dyn_cast<BinaryOperator>(stmt); op && op->getOpcode() == BO_Assign) {
        lhs = op->getLHS();
        rhs = op->getRHS();
    } else if (const auto* call = dyn_cast<CXXOperatorCallExpr>(stmt);
               call && call->getOperator() == OO_Equal && call->getNumArgs() == 2) {
        lhs = call->getArg(0);
        rhs = call->getArg(1);
    }
    if (!lhs) return std::nullopt;
    const auto* member = dyn_cast<MemberExpr>(lhs->IgnoreParenImpCasts());
    if (!member || member->isArrow() || !as_ref_to(member->getBase(), var)) return std::nullopt;
    const auto* field = dyn_cast<FieldDecl>(member->getMemberDecl());
    if (!field || mentions(rhs, var)) return std::nullopt;
    return FieldAssignment{.stmt = original, .field = field, .value = rhs};
}

// `var.push_back(value);` (or another configured mutator) not mentioning var.
bool is_mutator_call(const Stmt* stmt, const VarDecl* var, ArrayRef<StringRef> names) {
    const auto* call = dyn_cast<CXXMemberCallExpr>(strip(stmt));
    if (!call || !as_ref_to(call->getImplicitObjectArgument(), var)) return false;
    const auto* method = call->getMethodDecl();
    if (!method || !method->getIdentifier()) return false;
    if (std::find(names.begin(), names.end(), method->getName()) == names.end()) return false;
    return std::none_of(call->arg_begin(), call->arg_end(),
                        [var](const Expr* arg) { return mentions(arg, var); });
}

// `T x;`: no initializer, or the implicit default-constructor call Clang
// records for it (init style CallInit, no source text). Not `T x{}`,
// `T x = T()` or `T x(args)`.
bool is_default_initialized(const VarDecl* var) {
    const Expr* init = var->getInit();
    if (!init) return true;
    const auto* construct = dyn_cast<CXXConstructExpr>(init->IgnoreImplicit());
    return construct && !isa<CXXTemporaryObjectExpr>(construct) && construct->getNumArgs() == 0 &&
           !construct->isListInitialization() && construct->getParenOrBraceRange().isInvalid();
}

}  // namespace

DefaultThenAssignCheck::DefaultThenAssignCheck(StringRef name, ClangTidyContext* context)
    : ClangTidyCheck(name, context),
      min_assignments_(Options.get("MinAssignments", default_min_assignments)),
      min_mutator_calls_(Options.get("MinMutatorCalls", default_min_mutator_calls)),
      mutator_names_raw_(Options.get("MutatorNames", default_mutator_names).str()) {
    SmallVector<StringRef> parts;
    StringRef(mutator_names_raw_).split(parts, ';', -1, false);
    mutator_names_.assign(parts.begin(), parts.end());
}

void DefaultThenAssignCheck::storeOptions(ClangTidyOptions::OptionMap& opts) {
    Options.store(opts, "MinAssignments", min_assignments_);
    Options.store(opts, "MinMutatorCalls", min_mutator_calls_);
    Options.store(opts, "MutatorNames", mutator_names_raw_);
}

void DefaultThenAssignCheck::registerMatchers(MatchFinder* finder) {
    finder->addMatcher(compoundStmt(unless(isExpansionInSystemHeader())).bind("block"), this);
}

void DefaultThenAssignCheck::check(const MatchFinder::MatchResult& result) {
    const auto* block = result.Nodes.getNodeAs<CompoundStmt>("block");
    const SourceManager& sm = *result.SourceManager;
    const LangOptions& lang = result.Context->getLangOpts();
    const auto body = block->body();

    for (auto it = body.begin(); it != body.end(); ++it) {
        const auto* decl_stmt = dyn_cast<DeclStmt>(*it);
        if (!decl_stmt || !decl_stmt->isSingleDecl()) continue;
        const auto* var = dyn_cast<VarDecl>(decl_stmt->getSingleDecl());
        if (!var || !var->hasLocalStorage() || !is_default_initialized(var)) continue;
        const auto* record = var->getType()->getAsCXXRecordDecl();
        if (!record || var->getLocation().isMacroID()) continue;

        SmallVector<FieldAssignment> assignments;
        unsigned mutator_calls = 0;
        auto next = std::next(it);
        if (record->isAggregate()) {
            for (; next != body.end(); ++next) {
                auto assignment = field_assignment(*next, var);
                if (!assignment) break;
                assignments.push_back(*assignment);
            }
        } else {
            for (; next != body.end() && is_mutator_call(*next, var, mutator_names_); ++next) ++mutator_calls;
        }

        if (assignments.size() >= min_assignments_) {
            auto diag = this->diag(var->getLocation(),
                                   "%0 is default-constructed and then assigned field by field; "
                                   "initialize it in one braced expression")
                        << var;
            // Fix-it only when each field is assigned once: a designated
            // initializer cannot name a field twice, which is the point.
            SmallVector<FieldAssignment> ordered(assignments);
            std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
                return a.field->getFieldIndex() < b.field->getFieldIndex();
            });
            const bool unique = std::adjacent_find(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
                                    return a.field == b.field;
                                }) == ordered.end();
            if (unique && lang.CPlusPlus20) {
                std::string init = "{";
                for (const auto& a : ordered) {
                    if (init.size() > 1) init += ", ";
                    init += ".";
                    init += a.field->getName();
                    init += " = ";
                    init += Lexer::getSourceText(CharSourceRange::getTokenRange(a.value->getSourceRange()), sm, lang);
                }
                init += "}";
                diag << FixItHint::CreateInsertion(Lexer::getLocForEndOfToken(var->getLocation(), 0, sm, lang), init);
                for (const auto& a : assignments) {
                    const SourceLocation after_semi =
                        Lexer::findLocationAfterToken(a.stmt->getEndLoc(), tok::semi, sm, lang, true);
                    if (after_semi.isValid()) diag << FixItHint::CreateRemoval(removal_range(a.stmt, after_semi, sm));
                }
            }
            it = std::prev(next);
        } else if (mutator_calls >= min_mutator_calls_) {
            diag(var->getLocation(),
                 "%0 is default-constructed and then filled by %1 mutator calls; "
                 "initialize it with a braced list")
                << var << mutator_calls;
            it = std::prev(next);
        }
    }
}

}  // namespace clang::tidy::wrocpp
