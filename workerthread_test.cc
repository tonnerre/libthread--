/**
 * Tests for the worker thread implementation.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <google/protobuf/stubs/common.h>
#include "threadpool.h"

namespace threadpp
{
namespace testing
{
using ::testing::InitGoogleMock;
using ::testing::Return;
using google::protobuf::NewCallback;

class MockThreadQueue : public ThreadQueue
{
public:
	MOCK_METHOD1(Add, void(google::protobuf::Closure* t));

	MOCK_METHOD0(GetNextTask, google::protobuf::Closure*());

	MOCK_METHOD0(WaitUntilEmpty, void());

	MOCK_METHOD0(Clear, void());
};


class ThreadCanceller
{
public:
	ThreadCanceller() {}

	void CancelThread(WorkerThread** t)
	{
		(*t)->Cancel();
	}
};

class WorkerThreadTest : public ::testing::Test
{
};

TEST_F(WorkerThreadTest, RunAndCancelSelf)
{
	int fake_argc = 0;
	char*  fake_argv = {};
	ThreadCanceller c;
	MockThreadQueue q;
	WorkerThread* t;

	InitGoogleMock(&fake_argc, &fake_argv);

	EXPECT_CALL(q, GetNextTask())
		.WillOnce(Return(NewCallback(&c,
						&ThreadCanceller::CancelThread,
						&t)))
		.RetiresOnSaturation();

	t = new WorkerThread(&q);
	delete t;  // Should wait for the thread to finish.
}

}  // namespace testing
}  // namespace threadpp
