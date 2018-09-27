/* MIT License
 *
 * Copyright (c) 2018 Gilang Mentari Hamidy (gilang.hamidy@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <tuple>
#include <type_traits>

// Metaprogram to detect if a type  has a call operator overload
template<typename T>
class HasCallOperator
{
	typedef char Correct;
	typedef long Incorrect;
	template<typename TTest> static Correct Test(decltype(&TTest::operator()));
	template<typename TTest> static Incorrect Test(...);
public:
	static constexpr bool Value = sizeof(Test<T>(0)) == sizeof(Correct);
};

// Metaprogram to detect if a type is a member function pointer
template<typename TFuncType>
struct MemberFunction { };

template<typename TClass, typename TReturn, typename... TArgs>
struct MemberFunction<TReturn (TClass::*)(TArgs...)>
{
    // Arity is the total number of parameter
	static constexpr auto Arity = sizeof...(TArgs);

	typedef TReturn ReturnType;
	typedef TClass	DeclaringType;

    // Args will be used to obtain the parameter type of specified index
	template<size_t idx>
	using Args = typename std::tuple_element<idx, std::tuple<TArgs...>>::type;

	typedef std::tuple<TArgs...> ArgsTuple;
	typedef std::tuple<typename std::decay<TArgs>::type...> ArgsTupleDecay;
};

template<typename TClass, typename TReturn>
struct MemberFunction<TReturn (TClass::*)()>
{
// Arity is the total number of parameter
	static constexpr auto Arity = 0;

	typedef TReturn ReturnType;
	typedef TClass	DeclaringType;

    // Args will be used to obtain the parameter type of specified index
	template<size_t idx>
	using Args = void;
};

template<typename TClass, typename TReturn, typename... TArgs>
struct MemberFunction<TReturn (TClass::*)(TArgs...) const> : 
	MemberFunction<TReturn (TClass::*)(TArgs...)> { };

// Metaprogram to detect if a type is a callable object
// Rule: If a type has a a call operator, then it is considered a callable object
template<typename T, bool = HasCallOperator<T>::Value>
struct CallableObject;

template<typename T>
struct CallableObject<T, false>
{
	static constexpr bool Callable = false;
};

template<typename T>
struct CallableObject<T, true> : MemberFunction<decltype(&T::operator())>
{
	static constexpr bool Callable = true;
};


// EDSL Definitions

template<typename TLambda, typename TCompleteLambda>
struct AsyncOperand
{
    using IntrospectLambda = CallableObject<TLambda>; // <--- Include the introspect metaprogram here
    using IntrospectCompleteLambda = CallableObject<TCompleteLambda>;

    // Validation rules in the class definition
    static constexpr bool validation = (std::is_same_v<typename IntrospectLambda::ReturnType, void> && IntrospectCompleteLambda::Arity == 0)
                                     || std::is_same_v<typename IntrospectLambda::ReturnType, typename IntrospectCompleteLambda::template Args<0>>;

    static_assert(validation, "The asynchronous return value and complete lambda has different parameter");

    TLambda lambda;
    TCompleteLambda completeLambda;
};

template<typename TCompleteLambda>    
struct CompleteOperand
{
    using Introspect = CallableObject<TCompleteLambda>; // <--- Include the introspect metaprogram here

    // Validation rules in the class definition
    static_assert(Introspect::Arity == 1 || Introspect::Arity == 0, "Incorrect amount of parameter in complete lambda");
    static_assert(std::is_same_v<typename Introspect::ReturnType, void>, "The complete lambda cannot return any value");

    TCompleteLambda lambda;
};

class AsyncBuilder
{
public:
    template<typename TLambda>
    void operator&(TLambda l)
    {
        std::cout << "Async with lambda\n";
    }

    template<typename TLambda, typename TCompleteLambda>
    void operator&(AsyncOperand<TLambda, TCompleteLambda> operand)
    {
        std::cout << "Async with AsyncOperand\n";
    }
};

class CompleteBuilder
{
public:
    template<typename TCompleteLambda>
    CompleteOperand<TCompleteLambda> operator*(TCompleteLambda complete)
    {
        std::cout << "Complete with lambda\n";
        return { complete };
    }
};

template<typename TLambda, typename TCompleteLambda>
AsyncOperand<TLambda, TCompleteLambda> operator>>(TLambda l, CompleteOperand<TCompleteLambda> c)
{
    std::cout << "Combining CompleteOperand and async lambda to AsyncOperand\n";
    return { l, c.lambda };
}

#define tfc_async AsyncBuilder() & [] ()
#define tfc_async_complete >> CompleteBuilder() * []

int main()
{
    std::cout << "Hello" << std::endl;

    tfc_async
    {
        std::cout << "Hello" << std::endl;
    }
    tfc_async_complete
    {

    };

    std::cout << "Done" << std::endl;

    return 0;
}


