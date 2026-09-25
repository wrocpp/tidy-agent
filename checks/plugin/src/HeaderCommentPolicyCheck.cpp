#include "HeaderCommentPolicyCheck.h"

#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"

namespace clang::tidy::wrocpp {

namespace {

constexpr StringRef default_header_regex = R"(\.(h|hh|hpp|hxx|ipp|tpp|inl)$)";
constexpr StringRef default_pointer_regex = R"((see|in|from) commit [0-9a-f]{7,40}|git blame|git log -)";
// llvm::Regex is POSIX extended: no \b, so word ends are spelled out.
constexpr StringRef default_narration_regex =
    R"(used to (be|return|have|take|call|use|do)([^a-z]|$)|an earlier version|previous(ly)? (version|implementation)|version [0-9]+ of)";

}  // namespace

class HeaderCommentPolicyCheck::Handler : public CommentHandler {
public:
    Handler(HeaderCommentPolicyCheck& check, StringRef header, StringRef pointer, StringRef narration)
        : check_(check),
          header_(header),
          pointer_(pointer, llvm::Regex::IgnoreCase),
          narration_(narration, llvm::Regex::IgnoreCase) {}

    bool HandleComment(Preprocessor& pp, SourceRange range) override {
        const SourceManager& sm = pp.getSourceManager();
        const SourceLocation begin = range.getBegin();
        if (begin.isMacroID() || sm.isInSystemHeader(begin)) return false;
        if (!header_.match(sm.getFilename(begin))) return false;
        const StringRef text = Lexer::getSourceText(CharSourceRange::getCharRange(range), sm, pp.getLangOpts());
        if (pointer_.match(text))
            check_.diag(begin, "header comment points at version control; that belongs in the commit message");
        else if (narration_.match(text))
            check_.diag(begin, "header comment narrates the code's history; that belongs in the commit message");
        return false;
    }

private:
    HeaderCommentPolicyCheck& check_;
    llvm::Regex header_;
    llvm::Regex pointer_;
    llvm::Regex narration_;
};

HeaderCommentPolicyCheck::HeaderCommentPolicyCheck(StringRef name, ClangTidyContext* context)
    : ClangTidyCheck(name, context),
      header_regex_(Options.get("HeaderRegex", default_header_regex).str()),
      pointer_regex_(Options.get("PointerRegex", default_pointer_regex).str()),
      narration_regex_(Options.get("NarrationRegex", default_narration_regex).str()),
      handler_(std::make_unique<Handler>(*this, header_regex_, pointer_regex_, narration_regex_)) {}

HeaderCommentPolicyCheck::~HeaderCommentPolicyCheck() = default;

void HeaderCommentPolicyCheck::storeOptions(ClangTidyOptions::OptionMap& opts) {
    Options.store(opts, "HeaderRegex", header_regex_);
    Options.store(opts, "PointerRegex", pointer_regex_);
    Options.store(opts, "NarrationRegex", narration_regex_);
}

void HeaderCommentPolicyCheck::registerPPCallbacks(const SourceManager&, Preprocessor* pp, Preprocessor*) {
    pp->addCommentHandler(handler_.get());
}

}  // namespace clang::tidy::wrocpp
