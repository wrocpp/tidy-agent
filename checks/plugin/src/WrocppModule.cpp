#include "DefaultThenAssignCheck.h"
#include "EscapingRefCaptureCheck.h"
#include "HeaderCommentPolicyCheck.h"
#include "NeedlessSharedPtrCheck.h"

#include "clang-tidy/ClangTidyModule.h"

namespace clang::tidy::wrocpp {

class WrocppModule : public ClangTidyModule {
public:
    void addCheckFactories(ClangTidyCheckFactories& factories) override {
        factories.registerCheck<DefaultThenAssignCheck>("wrocpp-default-then-assign");
        factories.registerCheck<EscapingRefCaptureCheck>("wrocpp-escaping-ref-capture");
        factories.registerCheck<NeedlessSharedPtrCheck>("wrocpp-needless-shared-ptr");
        factories.registerCheck<HeaderCommentPolicyCheck>("wrocpp-header-comment-policy");
    }
};

}  // namespace clang::tidy::wrocpp

namespace clang::tidy {

// Registered under a name clang-tidy lists in --list-checks once loaded with -load.
static ClangTidyModuleRegistry::Add<wrocpp::WrocppModule>
    WrocppModuleRegistration("wrocpp-module", "Checks for recurring AI-agent mistakes (tidy-agent).");

}  // namespace clang::tidy
