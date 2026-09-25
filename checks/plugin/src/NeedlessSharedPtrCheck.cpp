#include "NeedlessSharedPtrCheck.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang::tidy::wrocpp {

namespace {

// The first parent of `node` that is not an implicit cast or parentheses.
DynTypedNode parent_skipping_implicit(ASTContext& context, const Expr* node) {
    DynTypedNode current = DynTypedNode::create(*node);
    while (true) {
        const auto parents = context.getParents(current);
        if (parents.empty()) return {};
        const DynTypedNode parent = parents[0];
        if (const auto* expr = parent.get<Expr>(); expr && (isa<ImplicitCastExpr>(expr) || isa<ParenExpr>(expr))) {
            current = parent;
            continue;
        }
        return parent;
    }
}

// A use that reads through the pointer without sharing ownership.
bool is_non_owning_use(ASTContext& context, const DeclRefExpr* ref) {
    const DynTypedNode parent = parent_skipping_implicit(context, ref);
    if (const auto* op = parent.get<CXXOperatorCallExpr>()) {
        const auto kind = op->getOperator();
        return (kind == OO_Arrow || kind == OO_Star) && op->getNumArgs() >= 1 &&
               op->getArg(0)->IgnoreParenImpCasts() == ref;
    }
    if (const auto* member = parent.get<MemberExpr>()) {
        const auto* method = dyn_cast<CXXMethodDecl>(member->getMemberDecl());
        if (!method) return false;
        if (isa<CXXConversionDecl>(method)) return method->getReturnType()->isBooleanType();
        return method->getIdentifier() && method->getName() == "get";
    }
    return false;
}

}  // namespace

void NeedlessSharedPtrCheck::registerMatchers(MatchFinder* finder) {
    const auto shared_ptr = hasUnqualifiedDesugaredType(
        recordType(hasDeclaration(classTemplateSpecializationDecl(hasName("::std::shared_ptr")))));
    finder->addMatcher(
        varDecl(hasLocalStorage(), unless(parmVarDecl()), hasType(shared_ptr),
                hasInitializer(ignoringImplicit(
                    callExpr(callee(functionDecl(hasName("::std::make_shared"))),
                             callee(expr(ignoringImplicit(declRefExpr().bind("callee"))))))),
                hasAncestor(functionDecl(hasBody(compoundStmt().bind("body")))),
                unless(isExpansionInSystemHeader()))
            .bind("var"),
        this);
}

void NeedlessSharedPtrCheck::check(const MatchFinder::MatchResult& result) {
    const auto* var = result.Nodes.getNodeAs<VarDecl>("var");
    const auto* body = result.Nodes.getNodeAs<CompoundStmt>("body");
    const auto* callee = result.Nodes.getNodeAs<DeclRefExpr>("callee");
    ASTContext& context = *result.Context;

    const auto refs = match(findAll(declRefExpr(to(varDecl(equalsNode(var)))).bind("ref")), *body, context);
    if (refs.empty()) return;
    for (const auto& bound : refs)
        if (!is_non_owning_use(context, bound.getNodeAs<DeclRefExpr>("ref"))) return;

    auto diag = this->diag(var->getLocation(),
                           "%0 is a shared_ptr that is never shared; use std::unique_ptr or a value")
                << var;
    const bool declared_auto = var->getType()->getContainedAutoType() != nullptr ||
                               isa<AutoType>(var->getTypeSourceInfo()->getType().getTypePtr());
    if (declared_auto && callee->getNameInfo().getSourceRange().isValid())
        diag << FixItHint::CreateReplacement(callee->getNameInfo().getSourceRange(), "make_unique");
}

}  // namespace clang::tidy::wrocpp
