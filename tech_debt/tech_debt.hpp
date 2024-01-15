/*
Tech debt of choose which method to use for Event

*/




// 1. operation overlapping between allocating memory and using a queue with emplace_back-like method

concurrentqueue::ConcurrentQueue<MyType> queue;
MyType myObject;
//usually enqueue a object
queue.enqueue(myObject);

// emplace_back-like : Enqueue a new MyType object constructed in place
queue.enqueue(MyTypeConstructorArg1, MyTypeConstructorArg2);

// usually i create a object first using je_malloc()
// this case, i manage memory of the object, but using emplace_back the container manage the memory.
// the over-lapping is both cases, there is memory allocation.
// to reduce the redundency may improve performance







//1. Structured Union with Manual Type Management
enum EventType {
    TICK, ORDER, TRADE
};

struct TradeData { /* Trade data members */ };
struct OrderData { /* Order data members */ };

struct Event {
    EventType etype;
    union Payload {
        TradeData tradeData;
        OrderData orderData;

        Payload() {}  // Constructor
        ~Payload() {} // Destructor
    } payload;

    ~Event() {
        // Manually destruct the payload based on the type
        switch (etype) {
            case TRADE: payload.tradeData.~TradeData(); break;
            case ORDER: payload.orderData.~OrderData(); break;
            default: break;
        }
    }
};

//2. Inline Storage with Placement New
#include <new> // For placement new
#include <type_traits> // For std::aligned_storage

enum EventType {
    TICK, ORDER, TRADE
};

struct Event {
    EventType etype;
    std::aligned_storage_t<sizeof(TradeData), alignof(TradeData)> storage;

    template<typename T>
    T* getPayload() {
        return reinterpret_cast<T*>(&storage);
    }

    template<typename T, typename... Args>
    void emplacePayload(Args&&... args) {
        new (&storage) T(std::forward<Args>(args)...);
    }

    ~Event() {
        // Manually destruct the payload
        // Call the destructor of the specific payload type
    }
};



/*
Memory pool

*/
#include <memory>

class MemoryPool {
    // Implementation of a memory pool that allocates fixed-size blocks
};

struct Event {
    EventType etype;
    // Payload as in the previous examples

    static MemoryPool pool;

    void* operator new(size_t size) {
        return pool.allocate(size);
    }

    void operator delete(void* pointer) {
        pool.deallocate(pointer);
    }
};

MemoryPool Event::pool; // Define the static memory pool


/*

I have some questions
1 . in your example Structured Union with Manual Type Management  i dont undersatnd how this line assign TradeData to Event.payload. and why use new to allocate on heap not stack? is it a pointer?
new (&e.payload.tradeData) TradeData();  
2.  could you explain what aligned_storage_t, alignofreinterpret_cast ,std::forward,alignof does? and why use them?
3. i dont uderstand how this line works: new (&storage) T(std::forward<Args>(args)...);

ChatGPT
Sure, I'll address each of your questions to clarify these concepts:

1. Structured Union with Manual Type Management
In the example with Structured Union, the line new (&e.payload.tradeData) TradeData(); is using placement new. This is not allocating memory on the heap; instead, it constructs an object in pre-allocated memory.

Placement New: It's a form of new operator in C++ that constructs an object on a pre-allocated buffer. It allows you to call the constructor and initialize an object at a specific memory location.

cpp
Copy code
new (&e.payload.tradeData) TradeData();
This line constructs a TradeData object in the space allocated for tradeData within the union payload of e. Here, &e.payload.tradeData provides the address where the TradeData object should be constructed. This is not heap allocation; it's constructing the object in the space already allocated for the union.

2. Explanation of std::aligned_storage, alignof, reinterpret_cast, and std::forward
std::aligned_storage_t: This is a type trait that defines a type with a specified size and alignment. It's used to create storage for an object without initializing it. This is useful for placement new, where you want to control when and how an object is constructed.

alignof: This operator returns the alignment requirement of a type. Alignment refers to how bytes are arranged and accessed in memory. Some types require specific alignment for efficient or correct operation on certain hardware.

reinterpret_cast: This cast operator is used to convert one pointer type to another. It allows you to treat a block of memory as a pointer of a different type. In the context of std::aligned_storage, it's used to treat the raw storage as a pointer to a specific type.

std::forward: This function is used in template forwarding to preserve the value category of function arguments. It's part of perfect forwarding, which allows a function template to forward arguments to another function keeping their lvalue/rvalue nature.

3. Understanding new (&storage) T(std::forward<Args>(args)...);
This line is using placement new along with perfect forwarding. It constructs an object of type T in the space provided by storage.

new (&storage) T: This is placement new. It constructs an object of type T in the memory addressed by &storage.

std::forward<Args>(args)...: This part is forwarding the constructor arguments to T's constructor. It preserves whether each argument is an lvalue or rvalue, which is important for ensuring that move semantics and copy semantics are applied correctly.

In summary, this line is constructing an object of type T with the provided arguments in the pre-allocated storage. This allows dynamic control over object construction without heap allocation.

Each of these techniques serves specific purposes in C++, particularly in scenarios requiring low-level memory management, efficient object construction, and type manipulations, which are common in high-performance and system-level programming.

User
in this example:
#include <new> // For placement new
#include <type_traits> // For std::aligned_storage

enum EventType {
    TICK, ORDER, TRADE
};

struct Event {
    EventType etype;
    std::aligned_storage_t<sizeof(TradeData), alignof(TradeData)> storage;

    template<typename T>
    T* getPayload() {
        return reinterpret_cast<T*>(&storage);
    }

    template<typename T, typename... Args>
    void emplacePayload(Args&&... args) {
        new (&storage) T(std::forward<Args>(args)...);
    }

    ~Event() {
        // Manually destruct the payload
        // Call the destructor of the specific payload type
    }
};

templete is used, this would mean that creating Event object of different payload types using this code would result in creating different Event objects and those objects cannot be stored in a std::queue<Events> right?
ChatGPT
In the provided example, the Event class uses templates for the emplacePayload method, but it does not make Event itself a template class. The Event class is a non-template class, meaning all Event objects are of the same type regardless of the payload they contain. This allows you to store them in a container like std::queue<Event> without any issues.

Key Points
Non-Template Class: The Event class itself is not a template class. It has a fixed type regardless of the payload it contains.

Template Methods: The emplacePayload method is a template method. This allows you to construct different types of payloads within the Event object, but it does not change the type of the Event object itself.

Storage Mechanism: The Event class uses std::aligned_storage to reserve raw storage that can accommodate any of the payload types. This storage is type-agnostic.

Same Event Type: Since the Event class type remains constant, you can store multiple Event objects with different payloads in a single std::queue<Event>.

Example of Storing in std::queue
You can store Event objects with different payloads in a std::queue<Event> like so:

cpp
Copy code
std::queue<Event> eventQueue;

Event tradeEvent;
tradeEvent.etype = TRADE;
tradeEvent.emplacePayload<TradeData>(/* trade data args );
eventQueue.push(tradeEvent);

Event orderEvent;
orderEvent.etype = ORDER;
orderEvent.emplacePayload<OrderData>(/* order data args );
eventQueue.push(orderEvent);
In this example, both tradeEvent and orderEvent are of the same Event type, even though they contain different payloads. They can be safely stored in the same queue.

Conclusion
In your case, using template methods within a non-template class (Event) provides the flexibility to handle various payload types while maintaining a consistent Event type, suitable for storage in standard containers like std::queue. This approach offers a balance between dynamic payload handling and the ability to use standard containers without the complexity of a templated class.
*/









