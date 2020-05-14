#include "Eco/ManualExecutor.hpp"
#include "Eco/Task.hpp"

#include <cstdio>

static Eco::ManualExecutor g_executor;

static Eco::Task<void> MyTask(Eco::TaskContext&, const char* s)
{
	puts(s);
	co_return;
}

static Eco::TaskContext g_myTaskContext1(g_executor);
static Eco::TaskContext g_myTaskContext2(g_executor);

int main()
{
	//while (!IsDebuggerPresent());

	MyTask(g_myTaskContext1, "hello").Start();
	MyTask(g_myTaskContext2, "world").Start();
	printf("%llu\n", g_executor.Execute());
}
