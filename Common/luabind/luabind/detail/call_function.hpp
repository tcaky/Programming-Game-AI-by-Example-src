// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.


#if !BOOST_PP_IS_ITERATING

#ifndef LUABIND_CALL_FUNCTION_HPP_INCLUDED
#define LUABIND_CALL_FUNCTION_HPP_INCLUDED

#include <luabind/config.hpp>

#include <boost/mpl/if.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/mpl/or.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>

#include <luabind/detail/convert_to_lua.hpp>
#include <luabind/detail/error.hpp>

namespace luabind
{
	namespace detail
	{



		// if the proxy_function_caller returns non-void
			template<class Ret, class Tuple>
			class proxy_function_caller
			{
//			friend class luabind::object;
			public:

				proxy_function_caller(lua_State* L, const char* name, const Tuple args)
					: m_state(L)
					, m_fun_name(name)
					, m_args(args)
					, m_called(false)
				{
				}

				proxy_function_caller(const proxy_function_caller& rhs)
					: m_state(rhs.m_state)
					, m_fun_name(rhs.m_fun_name)
					, m_args(rhs.m_args)
					, m_called(rhs.m_called)
				{
					rhs.m_called = true;
				}

				~proxy_function_caller()
				{
					if (m_called) return;

					m_called = true;
					lua_State* L = m_state;;

					// get the function
					lua_pushstring(L, m_fun_name);
					lua_gettable(L, LUA_GLOBALSINDEX);

					push_args_from_tuple<1>::apply(L, m_args);
					if (lua_pcall(L, boost::tuples::length<Tuple>::value, 0, 0))
					{ 
#ifndef LUABIND_NO_EXCEPTIONS
						throw luabind::error(L);
#else
						error_callback_fun e = detail::error_callback::get().err;
						if (e) e(L);

						assert(0 && "the lua function threw an error and exceptions are disabled."
									" If you want to handle the error you can use luabind::set_error_callback()");
						std::terminate();

#endif
					}
				}

				operator Ret()
				{
					typename default_policy::template generate_converter<Ret, lua_to_cpp>::type converter;

					m_called = true;
					lua_State* L = m_state;;
					detail::stack_pop p(L, 1); // pop the return value

					// get the function
					lua_pushstring(L, m_fun_name);
					lua_gettable(L, LUA_GLOBALSINDEX);

					push_args_from_tuple<1>::apply(L, m_args);
					if (lua_pcall(L, boost::tuples::length<Tuple>::value, 1, 0))
					{ 
#ifndef LUABIND_NO_EXCEPTIONS
						throw luabind::error(L); 
#else
						error_callback_fun e = detail::error_callback::get().err;
						if (e) e(L);
	
						assert(0 && "the lua function threw an error and exceptions are disabled."
								" If you want to handle the error you can use luabind::set_error_callback()");
						std::terminate();
#endif
					}

#ifndef LUABIND_NO_ERROR_CHECKING

					if (converter.match(L, LUABIND_DECORATE_TYPE(Ret), -1) < 0)
					{
#ifndef LUABIND_NO_EXCEPTIONS
						throw cast_failed(L, LUABIND_TYPEID(Ret));
#else
						cast_failed_callback_fun e = detail::error_callback::get().cast;
						if (e) e(L, LUABIND_TYPEID(Ret));

						assert(0 && "the lua function's return value could not be converted."
									" If you want to handle the error you can use luabind::set_error_callback()");
						std::terminate();

#endif
					}
#endif
					return converter.apply(L, LUABIND_DECORATE_TYPE(Ret), -1);
				}

				template<class Policies>
				Ret operator[](const Policies& p)
				{
					typedef typename detail::find_conversion_policy<0, Policies>::type converter_policy;
					typename converter_policy::template generate_converter<Ret, lua_to_cpp>::type converter;

					m_called = true;
					lua_State* L = m_state;;
					detail::stack_pop popper(L, 1); // pop the return value

					// get the function
					lua_pushstring(L, m_fun_name);
					lua_gettable(L, LUA_GLOBALSINDEX);

					detail::push_args_from_tuple<1>::apply(L, m_args, p);
					if (lua_pcall(L, boost::tuples::length<Tuple>::value, 1, 0))
					{ 
#ifndef LUABIND_NO_EXCEPTIONS
						throw error(L);
#else
						error_callback_fun e = detail::error_callback::get().err;
						if (e) e(L);
	
						assert(0 && "the lua function threw an error and exceptions are disabled."
								" If you want to handle the error you can use luabind::set_error_callback()");
						std::terminate();
#endif
					}

#ifndef LUABIND_NO_ERROR_CHECKING

					if (converter.match(L, LUABIND_DECORATE_TYPE(Ret), -1) < 0)
					{
#ifndef LUABIND_NO_EXCEPTIONS
						throw cast_failed(L, LUABIND_TYPEID(Ret));
#else
						cast_failed_callback_fun e = detail::error_callback::get().cast;
						if (e) e(L, LUABIND_TYPEID(Ret));

						assert(0 && "the lua function's return value could not be converted."
									" If you want to handle the error you can use luabind::set_error_callback()");
						std::terminate();

#endif
					}
#endif
					return converter.apply(L, LUABIND_DECORATE_TYPE(Ret), -1);
				}

			private:

				lua_State* m_state;
				const char* m_fun_name;
				Tuple m_args;
				mutable bool m_called;

			};

		// if the proxy_member_caller returns void
			template<class Tuple>
			class proxy_function_void_caller
			{
			friend class luabind::object;
			public:

				proxy_function_void_caller(lua_State* L, const char* name, const Tuple args)
					: m_state(L)
					, m_fun_name(name)
					, m_args(args)
					, m_called(false)
				{
				}

				proxy_function_void_caller(const proxy_function_void_caller& rhs)
					: m_state(rhs.m_state)
					, m_fun_name(rhs.m_fun_name)
					, m_args(rhs.m_args)
					, m_called(rhs.m_called)
				{
					rhs.m_called = true;
				}

				~proxy_function_void_caller()
				{
					if (m_called) return;

					m_called = true;
					lua_State* L = m_state;;

					// get the function
					lua_pushstring(L, m_fun_name);
					lua_gettable(L, LUA_GLOBALSINDEX);

					push_args_from_tuple<1>::apply(L, m_args);
					if (lua_pcall(L, boost::tuples::length<Tuple>::value, 0, 0))
					{ 
#ifndef LUABIND_NO_EXCEPTIONS
						throw luabind::error(L);
#else
						error_callback_fun e = detail::error_callback::get().err;
						if (e) e(L);
	
						assert(0 && "the lua function threw an error and exceptions are disabled."
								" If you want to handle the error you can use luabind::set_error_callback()");
						std::terminate();
#endif
					}
				}

				template<class Policies>
				void operator[](const Policies& p)
				{
					m_called = true;
					lua_State* L = m_state;;

					// get the function
					lua_pushstring(L, m_fun_name);
					lua_gettable(L, LUA_GLOBALSINDEX);

					detail::push_args_from_tuple<1>::apply(L, m_args, p);
					if (lua_pcall(L, boost::tuples::length<Tuple>::value, 0, 0))
					{ 
#ifndef LUABIND_NO_EXCEPTIONS
						throw error(L);
#else
						error_callback_fun e = detail::error_callback::get().err;
						if (e) e(L);
	
						assert(0 && "the lua function threw an error and exceptions are disabled."
							" If you want to handle the error you can use luabind::set_error_callback()");
						std::terminate();
#endif
					}
				}

			private:

				lua_State* m_state;
				const char* m_fun_name;
				Tuple m_args;
				mutable bool m_called;

			};

	}

	#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/detail/call_function.hpp>, 1))
	#include BOOST_PP_ITERATE()

}

#endif // LUABIND_CALL_FUNCTION_HPP_INCLUDED

#elif BOOST_PP_ITERATION_FLAGS() == 1

#define LUABIND_TUPLE_PARAMS(z, n, data) const A##n *
#define LUABIND_OPERATOR_PARAMS(z, n, data) const A##n & a##n


	template<class Ret BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
	typename boost::mpl::if_<boost::is_void<Ret>
			, luabind::detail::proxy_function_void_caller<boost::tuples::tuple<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_TUPLE_PARAMS, _)> >
			, luabind::detail::proxy_function_caller<Ret, boost::tuples::tuple<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_TUPLE_PARAMS, _)> > >::type
	call_function(lua_State* L, const char* name BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_OPERATOR_PARAMS, _) )
	{
		typedef boost::tuples::tuple<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_TUPLE_PARAMS, _)> tuple_t;
#if BOOST_PP_ITERATION() == 0
		tuple_t args;
#else
		tuple_t args(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), &a));
#endif
		typedef typename boost::mpl::if_<boost::is_void<Ret>
			, luabind::detail::proxy_function_void_caller<boost::tuples::tuple<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_TUPLE_PARAMS, _)> >
			, luabind::detail::proxy_function_caller<Ret, boost::tuples::tuple<BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_TUPLE_PARAMS, _)> > >::type proxy_type;
		
		return proxy_type(L, name, args);
	}

#undef LUABIND_OPERATOR_PARAMS
#undef LUABIND_TUPLE_PARAMS


#endif

