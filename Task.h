#include <list>
#include <queue>
#include <memory>

class ITask;
class TaskPool;
class CTaskToNonClass;
template <class T> class CTaskToClass;
class TaskQueue;

// task를 생성해주는 도우미 함수
template <class T = CTaskToNonClass, typename ...Type>
ITask* make_task(Type... _Args);

// task를 삭제해주는 도우미 함수
void delete_task(ITask** pTask);

// 싱글턴 패턴
// 위의 두 도우미 함수에서만 사용
extern TaskPool* g_pTaskPool;

////////////////////////////
// Class Name: ITask
////////////////////////////
class ITask
{
public:
	virtual ~ITask();
	virtual bool Invoke(void* pArg, int index) = 0;
};

////////////////////////////////
// Class Name: CTaskToNonClass
////////////////////////////////
class CTaskToNonClass : public ITask
{
	template <class T, typename ...Type>
	friend ITask* make_task(Type... _Args);

public:
	virtual ~CTaskToNonClass();
	virtual bool Invoke(void* pArg, int index) override;

private:
	explicit CTaskToNonClass(void (*pFunc) (void*, int));
	explicit CTaskToNonClass() = default;

	void (*m_pFunc)(void*, int);
};


////////////////////////////////
// Class Name: CTaskToClass
////////////////////////////////
template <class T>
class CTaskToClass : public ITask
{
	template <class T, typename ...Type>
	friend ITask* make_task(Type... _Args);

public:
	virtual ~CTaskToClass();
	virtual bool Invoke(void* pArg, int index) override;

private:
	explicit CTaskToClass(T* pInstance, void (T::* pFunc)(void*, int));
	explicit CTaskToClass() = default;

	T* m_pInstance;
	void (T::* m_pFunc)(void*, int);
};



////////////////////////////////
// Class Name: TaskPool
////////////////////////////////
class TaskPool final
{
	template <class T, typename ...Type>
	friend ITask* make_task(Type... _Args);
	friend void delete_task(ITask** pTask);

public:
	~TaskPool();
	static TaskPool* MakeTaskPool(size_t maxTaskNum, size_t maxTaskSize);
	inline void ResetPool() { m_Head = 0; }

private:
	TaskPool() = default;

	template <class T>
	ITask* GetTaskMemory();
	void ReturnTaskMemory(ITask** pTask);

	inline size_t GetFreeSizeInByte() { return m_Capacity - m_Head; }

	static TaskPool* m_pTaskPool;
	static size_t	      m_Head;
	static size_t	      m_Capacity;
	static unsigned char* m_FreeTaskList;
};



////////////////////////////////
// Class Name: TaskQueue
////////////////////////////////
class TaskQueue final
{
public:
	TaskQueue();
	~TaskQueue();

	void Push(ITask* pTask);
	ITask* Pop();
	inline bool Empty() const { return m_TaskQueue.empty(); }

private:
	std::queue<ITask*> m_TaskQueue;
};



// 함수 정의

template <class T>
CTaskToClass<T>::CTaskToClass(T* pInstance, void (T::* pFunc)(void*, int))
	: m_pInstance(pInstance)
	, m_pFunc(pFunc)
{
}

template <class T>
CTaskToClass<T>::~CTaskToClass()
{
	m_pInstance = nullptr;
	m_pFunc = nullptr;
}

template <class T>
bool CTaskToClass<T>::Invoke(void* pArg, int index)
{
#ifdef _DEBUG
	if (m_pFunc == nullptr)
		__debugbreak();
#endif 

	(m_pInstance->*m_pFunc)(pArg, index);
	return true;
}


template <class T>
ITask* TaskPool::GetTaskMemory()
{
	size_t availableSize = GetFreeSizeInByte();
	void* pMemory = static_cast<void*>(m_FreeTaskList + m_Head);

	// T 타입에 맞게 메모리를 정렬하여 반환합니다.
	if (std::align(alignof(T), sizeof(T), pMemory, availableSize))
	{
		T* pAlignedMemory = reinterpret_cast<T*>(pMemory);
		m_Head += sizeof(T);
		return pAlignedMemory;
	}
	else 
	{
#ifdef _DEBUG
		__debugbreak();
#endif 
		return nullptr;
	}
}


//////////////////////////////////////////////////////////////////////////
// Task 생성 도우미 함수
// - pAlignedMemory라는 내가 원하는 메모리 영역에 객체를 초기화 (placement new)
// - 해제 시 delete가 아닌 직접 소멸자를 호출해야 함(delete_task 함수 사용)
// - DO NOT USE 'delete' keyword for releasing pTask's memory.
// - Instead CALL the object's destructor
//////////////////////////////////////////////////////////////////////////
template <class T, typename... Type>
ITask* make_task(Type... _Args)
{
	ITask* pAlignedMemory = g_pTaskPool->GetTaskMemory<T>();

#pragma push_macro("new")
#undef new
	ITask* pTask = new (pAlignedMemory) T(_Args...);
#pragma pop_macro("new")

	return pTask;
}