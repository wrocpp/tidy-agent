#include "EscapingRefCaptureCheck.h"

#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/ADT/SmallVector.h"

using namespace clang::ast_matchers;

namespace clang::tidy::wrocpp {

namespace {

constexpr StringRef default_callable_types = "::std::function;::std::move_only_function;::std::copyable_function";

bool captures_by_reference(const LambdaExpr* lambda) {
    if (lambda->getCaptureDefault() == LCD_ByRef) return true;
    for (const LambdaCapture& capture : lambda->explicit_captures())
        if (capture.capturesVariable() && capture.getCaptureKind() == LCK_ByRef) return true;
    return false;
}

}  // namespace

EscapingRefCaptureCheck::EscapingRefCaptureCheck(StringRef name, ClangTidyContext* context)
    : ClangTidyCheck(name, context),
      callable_types_raw_(Options.get("CallableTypes", default_callable_types).str()) {
    SmallVector<StringRef> parts;
    StringRef(callable_types_raw_).split(parts, ';', -1, false);
    callable_types_.assign(parts.begin(), parts.end());
}

void EscapingRefCaptureCheck::storeOptions(ClangTidyOptions::OptionMap& opts) {
    Options.store(opts, "CallableTypes", callable_types_raw_);
}

void EscapingRefCaptureCheck::registerMatchers(MatchFinder* finder) {
    const auto lambda = lambdaExpr(unless(isExpansionInSystemHeader())).bind("lambda");
    // Returned directly (auto return type, or copy-elided).
    finder->addMatcher(returnStmt(hasReturnValue(ignoringImplicit(ignoringElidableConstructorCall(lambda)))), this);
    // Converted into a type-erased callable: returned, stored, or passed on.
    finder->addMatcher(
        cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(hasAnyName(callable_types_)))),
                         hasArgument(0, ignoringImplicit(lambda))),
        this);
}

void EscapingRefCaptureCheck::check(const MatchFinder::MatchResult& result) {
    const auto* lambda = result.Nodes.getNodeAs<LambdaExpr>("lambda");
    if (!captures_by_reference(lambda) || !reported_.insert(lambda).second) return;
    diag(lambda->getBeginLoc(),
         "lambda captures by reference and escapes the enclosing scope; capture by value, "
         "or keep the call inside the scope");
}

}  // namespace clang::tidy::wrocpp
