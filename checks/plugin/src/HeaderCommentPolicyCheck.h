#pragma once

#include "clang-tidy/ClangTidyCheck.h"

#include "llvm/Support/Regex.h"

#include <memory>
#include <string>

namespace clang::tidy::wrocpp {

/// Flags comments in header files that point at version control ("see commit
/// abc1234", "git blame") or narrate the code's history ("used to be",
/// "an earlier version"). That content belongs in the commit message.
///
/// Options:
///   HeaderRegex      files the policy applies to (default: common header suffixes)
///   PointerRegex     git pointers
///   NarrationRegex   past-tense history
class HeaderCommentPolicyCheck : public ClangTidyCheck {
public:
    HeaderCommentPolicyCheck(StringRef name, ClangTidyContext* context);
    ~HeaderCommentPolicyCheck() override;
    void storeOptions(ClangTidyOptions::OptionMap& opts) override;
    void registerPPCallbacks(const SourceManager& sm, Preprocessor* pp, Preprocessor* module_expander_pp) override;

private:
    class Handler;
    std::string header_regex_;
    std::string pointer_regex_;
    std::string narration_regex_;
    std::unique_ptr<Handler> handler_;
};

}  // namespace clang::tidy::wrocpp
