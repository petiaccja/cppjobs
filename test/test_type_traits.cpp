#include <catch.hpp>
#include <cppjobs/future.hpp>
#include <cppjobs/type_traits.hpp>



TEST_CASE("is_awaitable", "[Type traits]") {
	constexpr bool _int = cppjobs::awaitable<int>;
	static_assert(!_int);
	constexpr bool _future = cppjobs::awaitable<cppjobs::future<int>>;
	static_assert(_future);
}

TEST_CASE("await_result_t", "[Type traits]") {
	using type = cppjobs::await_result_t<cppjobs::future<int>>;
	static_assert(std::is_same_v<type, int>);
}


