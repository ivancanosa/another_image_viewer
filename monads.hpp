#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

// ******** Either monad

template<typename Left, typename Right>
using Either = std::variant<Left, Right>;

template<typename EitherT, typename F>
auto bindM(const EitherT& m, const F& f) -> EitherT {
    using Right = std::variant_alternative_t<1, EitherT>;
    if (std::holds_alternative<Right>(m)) {
        return std::invoke(f, std::get<Right>(m));
    }
    return m;
}

template<typename EitherT, typename F>
auto bindMRef(EitherT& m, const F& f) -> EitherT& {
    using Right = std::variant_alternative_t<1, EitherT>;
    if (std::holds_alternative<Right>(m)) {
        std::invoke(f, m);
    }
    return m;
}

template<typename Left, typename Right, typename F>
auto operator|(Either<Left, Right>&& m, F&& f) {
    return bindM(std::forward<Either<Left, Right>>(m), std::forward<F>(f));
}

template<typename Left, typename Right, typename F>
auto operator%(Either<Left, Right>&& m, F&& f) -> Either<Left, Right>& {
    return bindMRef(std::forward<Either<Left, Right>>(m), std::forward<F>(f));
}

// ******* Maybe monad

template<typename T> auto maybe(T&& v) -> std::optional<T> {
    return std::optional<std::decay_t<T>>(std::forward<T>(v));
}

template<typename T, typename F>
auto bindM(const std::optional<T>& m, const F& f)
    -> decltype(std::invoke(f, *m)) {
    if (m) {
        return std::invoke(f, *m);
    } else {
        return {};
    }
}

template<typename T, typename F>
auto bindMRef(std::optional<T>& m, const F& f) -> std::optional<T>& {
    if (m) {
        std::invoke(f, m);
    }
    return m;
}

template<typename T, typename F> auto operator|(std::optional<T>&& m, F&& f) {
    return bindM(std::forward<std::optional<T>>(m), std::forward<F>(f));
}

template<typename T, typename F>
auto operator%(std::optional<T>&& m, F&& f) -> std::optional<T>& {
    return bindMRef(std::forward<std::optional<T>>(m), std::forward<F>(f));
}

