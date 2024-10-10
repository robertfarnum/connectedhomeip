
#include "test/test.rpc.pb.h"

namespace test {

class TestImpl final: public pw_rpc::nanopb::Test::Service<TestImpl> {
    public:
        pw::Status DisplayText(const test_TextMessage& request, test_ErrorCode& response);
};

}

