#include "Task.h"

TaskPool* g_pTaskPool = nullptr;

enum eTask
{
	MAX_TASK_NUM = 100,
	MAX_TASK_SIZE = std::max(sizeof(CTaskToNonClass), sizeof(CTaskToClass<ITask>)),
};

/**********************************
 Class Name: ITask
**********************************/
ITask::~ITask()
{
}


/**********************************
 Class Name: CTaskToNonClass
**********************************/
CTaskToNonClass::CTaskToNonClass(void (*pFunc) (void*, int))
	: m_pFunc(pFunc)
{
}

CTaskToNonClass::~CTaskToNonClass()
{
	m_pFunc = nullptr;
}

bool CTaskToNonClass::Invoke(void* pArg, int index)
{
#ifdef _DEBUG
	if (m_pFunc == nullptr)
		__debugbreak();
#endif 

	m_pFunc(pArg, index);
	return true;
}


//////////////////////////////////////
// TaskPool의 static 멤버 변수 초기화
//////////////////////////////////////
TaskPool*	   TaskPool::m_pTaskPool = nullptr;
size_t		   TaskPool::m_Head = 0;
size_t		   TaskPool::m_Capacity = 0;
unsigned char* TaskPool::m_FreeTaskList = nullptr;


/**********************************
 Class Name: TaskPool
**********************************/
TaskPool::~TaskPool()
{
	delete[] m_FreeTaskList;
}

TaskPool* TaskPool::MakeTaskPool(size_t maxTaskNum, size_t maxTaskSize)
{
	if (m_pTaskPool != nullptr)
	{
		return m_pTaskPool;
	}

	m_pTaskPool = new TaskPool;
	m_Head = 0;
	m_Capacity = maxTaskSize * maxTaskNum;
	m_FreeTaskList = new unsigned char[m_Capacity];

#ifdef _DEBUG
	if (m_FreeTaskList == nullptr)
		__debugbreak();
#endif 

	return m_pTaskPool;
}


void TaskPool::ReturnTaskMemory(ITask** pTask)
{
	// placement new를 통해 할당했기 때문에
	// delete를 사용하면 안됨
	if (*pTask != nullptr)
	{
		(*pTask)->~ITask();
		*pTask = nullptr;
	}
}

/**********************************
 Class Name: TaskQueue
**********************************/
TaskQueue::TaskQueue()
{
	g_pTaskPool = TaskPool::MakeTaskPool(eTask::MAX_TASK_NUM, eTask::MAX_TASK_SIZE);
}

TaskQueue::~TaskQueue()
{
	//delete m_TaskPool;
}

void TaskQueue::Push(ITask* pTask)
{
	m_TaskQueue.push(pTask);
}

ITask* TaskQueue::Pop()
{
	ITask* pTask = m_TaskQueue.front();
	m_TaskQueue.pop();

	return pTask;
}



/////////////////////////
// Task 삭제 도우미 함수
/////////////////////////

// make_task로 생성된 인스턴스만 받아야 합니다.
void delete_task(ITask** pTask)
{
	g_pTaskPool->ReturnTaskMemory(pTask);
}