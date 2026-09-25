#pragma once

#include "clang-tidy/ClangTidyCheck.h"

#include <string>
#include <vector>

namespace clang::tidy::wrocpp {

/// Flags a lambda with a by-reference capture that outlives the call: it is
/// the operand of a return statement, or it is converted to a type-erased
/// callable (std::function by default) that can be stored and called later.
///
/// Options:
///   CallableTypes  semicolon-separated type-erased callables
///                  (default ::std::function;::std::move_only_function;::std::copyable_function)
class EscapingRefCaptureCheck : public ClangTidyCheck {
public:
    EscapingRefCaptureCheck(StringRef name, ClangTidyContext* context);
    void storeOptions(ClangTidyOptions::OptionMap& opts) override;
    void registerMatchers(ast_matchers::MatchFinder* finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult& result) override;
    bool isLanguageVersionSupported(const LangOptions& lang) const override { return lang.CPlusPlus11; }

private:
    std::string callable_types_raw_;
    std::vector<StringRef> callable_types_;
    llvm::DenseSet<const LambdaExpr*> reported_;
};

}  // namespace clang::tidy::wrocpp
