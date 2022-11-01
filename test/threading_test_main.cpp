
#include "threading_grind.h"
#include "async_pipe_test.h"

void threading_test_main()
{
	TEST_FUNCTION(test_async_pipe1);
	TEST_FUNCTION(test_async_pipe2);
	TEST_FUNCTION(test_async_pipe3);
	TEST_FUNCTION(test_thread_grind);
}

TEST_MAIN(threading_test_main);
