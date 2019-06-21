/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cassert>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace util
{

	template <typename T> class Delegate;

	template<class R, class ...A>
	class Delegate<R(A...)>
	{
		using stub_ptr_type = R(*)(void*, A&&...);

		Delegate(void* const o, stub_ptr_type const m) noexcept :
			m_object_ptr(o),
			m_stub_ptr(m)
		{
		}

	public:
		Delegate() :
			m_object_ptr(),
			m_deleter(),
			m_store_size()
		{
		}

		Delegate(Delegate const&) = default;

		Delegate(Delegate&&) = default;

		Delegate(::std::nullptr_t const) noexcept : Delegate() { }

		template <class C, typename =
			typename ::std::enable_if < ::std::is_class<C>{} > ::type >
			explicit Delegate(C const* const o) noexcept :
			m_object_ptr(const_cast<C*>(o))
		{
		}

		template <class C, typename =
			typename ::std::enable_if < ::std::is_class<C>{} > ::type >
			explicit Delegate(C const& o) noexcept :
			m_object_ptr(const_cast<C*>(&o))
		{
		}

		template <class C>
		Delegate(C* const object_ptr, R(C::* const method_ptr)(A...))
		{
			*this = from(object_ptr, method_ptr);
		}

		template <class C>
		Delegate(C* const object_ptr, R(C::* const method_ptr)(A...) const)
		{
			*this = from(object_ptr, method_ptr);
		}

		template <class C>
		Delegate(C& object, R(C::* const method_ptr)(A...))
		{
			*this = from(object, method_ptr);
		}

		template <class C>
		Delegate(C const& object, R(C::* const method_ptr)(A...) const)
		{
			*this = from(object, method_ptr);
		}

		template <
			typename T,
			typename = typename ::std::enable_if <
			!::std::is_same<Delegate, typename ::std::decay<T>::type>{}
			> ::type
		>
				Delegate(T&& f) :
				m_store(operator new(sizeof(typename ::std::decay<T>::type)),
					functor_deleter<typename ::std::decay<T>::type>),
				m_store_size(sizeof(typename ::std::decay<T>::type))
			{
				using functor_type = typename ::std::decay<T>::type;

				new (m_store.get()) functor_type(::std::forward<T>(f));

				m_object_ptr = m_store.get();

				m_stub_ptr = functor_stub<functor_type>;

				m_deleter = deleter_stub<functor_type>;
			}

			Delegate& operator=(Delegate const&) = default;

			Delegate& operator=(Delegate&&) = default;

			template <class C>
			Delegate& operator=(R(C::* const rhs)(A...))
			{
				return *this = from(static_cast<C*>(m_object_ptr), rhs);
			}

			template <class C>
			Delegate& operator=(R(C::* const rhs)(A...) const)
			{
				return *this = from(static_cast<C const*>(m_object_ptr), rhs);
			}

			template <
				typename T,
				typename = typename ::std::enable_if <
				!::std::is_same<Delegate, typename ::std::decay<T>::type>{}
				> ::type
			>
					Delegate& operator=(T&& f)
				{
					using functor_type = typename ::std::decay<T>::type;

					// Note that use_count is an approximation in multithreaded environments.
					if ((sizeof(functor_type) > m_store_size) || m_store.use_count() != 1)
					{
						m_store.reset(operator new(sizeof(functor_type)),
							functor_deleter<functor_type>);

						m_store_size = sizeof(functor_type);
					}
					else
					{
						m_deleter(m_store.get());
					}

					new (m_store.get()) functor_type(::std::forward<T>(f));

					m_object_ptr = m_store.get();

					m_stub_ptr = functor_stub<functor_type>;

					m_deleter = deleter_stub<functor_type>;

					return *this;
				}

				template <R(*const function_ptr)(A...)>
				static Delegate from() noexcept
				{
					return { nullptr, function_stub<function_ptr> };
				}

				template <class C, R(C::* const method_ptr)(A...)>
				static Delegate from(C* const object_ptr) noexcept
				{
					return { object_ptr, method_stub<C, method_ptr> };
				}

				template <class C, R(C::* const method_ptr)(A...) const>
				static Delegate from(C const* const object_ptr) noexcept
				{
					return { const_cast<C*>(object_ptr), const_method_stub<C, method_ptr> };
				}

				template <class C, R(C::* const method_ptr)(A...)>
				static Delegate from(C& object) noexcept
				{
					return { &object, method_stub<C, method_ptr> };
				}

				template <class C, R(C::* const method_ptr)(A...) const>
				static Delegate from(C const& object) noexcept
				{
					return { const_cast<C*>(&object), const_method_stub<C, method_ptr> };
				}

				template <typename T>
				static Delegate from(T&& f)
				{
					return ::std::forward<T>(f);
				}

				static Delegate from(R(*const function_ptr)(A...))
				{
					return function_ptr;
				}

				template <class C>
				using member_pair =
					::std::pair<C* const, R(C::* const)(A...)>;

				template <class C>
				using const_member_pair =
					::std::pair<C const* const, R(C::* const)(A...) const>;

				template <class C>
				static Delegate from(C* const object_ptr,
					R(C::* const method_ptr)(A...))
				{
					return member_pair<C>(object_ptr, method_ptr);
				}

				template <class C>
				static Delegate from(C const* const object_ptr,
					R(C::* const method_ptr)(A...) const)
				{
					return const_member_pair<C>(object_ptr, method_ptr);
				}

				template <class C>
				static Delegate from(C& object, R(C::* const method_ptr)(A...))
				{
					return member_pair<C>(&object, method_ptr);
				}

				template <class C>
				static Delegate from(C const& object,
					R(C::* const method_ptr)(A...) const)
				{
					return const_member_pair<C>(&object, method_ptr);
				}

				void reset() { m_stub_ptr = nullptr; m_store.reset(); }

				void reset_stub() noexcept { m_stub_ptr = nullptr; }

				void swap(Delegate& other) noexcept { ::std::swap(*this, other); }

				bool operator==(Delegate const& rhs) const noexcept
				{
					return (m_object_ptr == rhs.m_object_ptr) && (m_stub_ptr == rhs.m_stub_ptr);
				}

				bool operator!=(Delegate const& rhs) const noexcept
				{
					return !operator==(rhs);
				}

				bool operator<(Delegate const& rhs) const noexcept
				{
					return (m_object_ptr < rhs.m_object_ptr) ||
						((m_object_ptr == rhs.m_object_ptr) && (m_stub_ptr < rhs.m_stub_ptr));
				}

				bool operator==(::std::nullptr_t const) const noexcept
				{
					return !m_stub_ptr;
				}

				bool operator!=(::std::nullptr_t const) const noexcept
				{
					return m_stub_ptr;
				}

				explicit operator bool() const noexcept { return m_stub_ptr; }

				R operator()(A... args) const
				{
					//  assert(stub_ptr);
					return m_stub_ptr(m_object_ptr, ::std::forward<A>(args)...);
				}

	private:
		friend struct ::std::hash<Delegate>;

		using deleter_type = void(*)(void*);

		void* m_object_ptr;
		stub_ptr_type m_stub_ptr{};

		deleter_type m_deleter;

		::std::shared_ptr<void> m_store;
		::std::size_t m_store_size;

		template <class T>
		static void functor_deleter(void* const p)
		{
			static_cast<T*>(p)->~T();

			operator delete(p);
		}

		template <class T>
		static void deleter_stub(void* const p)
		{
			static_cast<T*>(p)->~T();
		}

		template <R(*function_ptr)(A...)>
		static R function_stub(void* const, A&&... args)
		{
			return function_ptr(::std::forward<A>(args)...);
		}

		template <class C, R(C::*method_ptr)(A...)>
		static R method_stub(void* const object_ptr, A&&... args)
		{
			return (static_cast<C*>(object_ptr)->*method_ptr)(
				::std::forward<A>(args)...);
		}

		template <class C, R(C::*method_ptr)(A...) const>
		static R const_method_stub(void* const object_ptr, A&&... args)
		{
			return (static_cast<C const*>(object_ptr)->*method_ptr)(
				::std::forward<A>(args)...);
		}

		template <typename>
		struct is_member_pair : std::false_type { };

		template <class C>
		struct is_member_pair< ::std::pair<C* const,
			R(C::* const)(A...)> > : std::true_type
		{
		};

		template <typename>
		struct is_const_member_pair : std::false_type { };

		template <class C>
		struct is_const_member_pair< ::std::pair<C const* const,
			R(C::* const)(A...) const> > : std::true_type
		{
		};

		template <typename T>
		static typename ::std::enable_if <
			!(is_member_pair<T>() ||
				is_const_member_pair<T>()),
			R
		> ::type
			functor_stub(void* const object_ptr, A&&... args)
		{
			return (*static_cast<T*>(object_ptr))(::std::forward<A>(args)...);
		}

		template <typename T>
		static typename ::std::enable_if <
			is_member_pair<T>() ||
			is_const_member_pair<T>(),
			R
		> ::type
			functor_stub(void* const object_ptr, A&&... args)
		{
			return (static_cast<T*>(object_ptr)->first->*
				static_cast<T*>(object_ptr)->second)(::std::forward<A>(args)...);
		}
	};
} /* util */

namespace std
{
	template <typename R, typename ...A>
	struct hash<::util::Delegate<R(A...)> >
	{
		size_t operator()(::util::Delegate<R(A...)> const& d) const noexcept
		{
			auto const seed(hash<void*>()(d.object_ptr_));

			return hash<typename ::util::Delegate<R(A...)>::stub_ptr_type>()(
				d.stub_ptr_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
	};
}