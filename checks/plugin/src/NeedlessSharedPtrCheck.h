#pragma once

#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::wrocpp {

/// Flags a local std::shared_ptr initialized with std::make_shared whose only
/// uses in the function are dereference (->, *), get(), and the bool
/// conversion. Any copy, move, capture, return or other use means ownership
/// may be shared, and the variable is left alone.
///
/// Fix-it, when the variable is declared with auto: make_shared -> make_unique.
class NeedlessSharedPtrCheck : public ClangTidyCheck {
public:
    NeedlessSharedPtrCheck(StringRef name, ClangTidyContext* context) : ClangTidyCheck(name, context) {}
    void registerMatchers(ast_matchers::MatchFinder* finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult& result) override;
    bool isLanguageVersionSupported(const LangOptions& lang) const override { return lang.CPlusPlus14; }
};

}  // namespace clang::tidy::wrocpp
