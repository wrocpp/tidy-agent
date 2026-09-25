#pragma once

#include "clang-tidy/ClangTidyCheck.h"

#include <string>
#include <vector>

namespace clang::tidy::wrocpp {

/// Flags an object that is default-constructed and then filled in by a run of
/// member assignments (aggregates) or mutator calls (containers), when the
/// whole value could be written as one braced expression.
///
/// Options:
///   MinAssignments   member assignments that make a run (default 2)
///   MinMutatorCalls  mutator calls that make a run (default 3)
///   MutatorNames     semicolon-separated method names
///                    (default push_back;emplace_back;insert;emplace;add)
///
/// Fix-it: for an aggregate whose run assigns each field at most once and
/// never reads the object, rewrites the declaration as a designated
/// initializer (C++20) and removes the assignments. Containers get no fix-it.
class DefaultThenAssignCheck : public ClangTidyCheck {
public:
    DefaultThenAssignCheck(StringRef name, ClangTidyContext* context);
    void storeOptions(ClangTidyOptions::OptionMap& opts) override;
    void registerMatchers(ast_matchers::MatchFinder* finder) override;
    void check(const ast_matchers::MatchFinder::MatchResult& result) override;
    bool isLanguageVersionSupported(const LangOptions& lang) const override { return lang.CPlusPlus; }

private:
    unsigned min_assignments_;
    unsigned min_mutator_calls_;
    std::string mutator_names_raw_;
    std::vector<StringRef> mutator_names_;
};

}  // namespace clang::tidy::wrocpp
