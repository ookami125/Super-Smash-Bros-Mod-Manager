#pragma once
#include <string>

template<typename T>
class ErrorOr
{
    public:
        ErrorOr(T data, bool error = false);
        ErrorOr(std::string errorString);
        ErrorOr(const ErrorOr<T>& other);
        ErrorOr(ErrorOr<T>&& other);
        ErrorOr<T>& operator=(const ErrorOr<T>& other);
        ErrorOr<T>& operator=(ErrorOr<T>&& other);
        ~ErrorOr();

        bool IsError() const;
        T& Get();
        const T& Get() const;
        std::string GetError() const;
    private:
        T data_;
        bool error_;
        std::string errorString_;
};

template <typename T>
inline ErrorOr<T>::ErrorOr(T data, bool error)
    : data_(data), error_(error)
{
}

template <typename T>
inline ErrorOr<T>::ErrorOr(std::string errorString)
    : error_(true), errorString_(errorString)
{
}

template <typename T>
inline ErrorOr<T>::ErrorOr(const ErrorOr<T>& other)
    : data_(other.data_), error_(other.error_), errorString_(other.errorString_)
{
}

template <typename T>
inline ErrorOr<T>::ErrorOr(ErrorOr<T>&& other)
    : data_(std::move(other.data_)), error_(other.error_), errorString_(std::move(other.errorString_))
{
}

template <typename T>
inline ErrorOr<T>& ErrorOr<T>::operator=(const ErrorOr<T>& other)
{
    data_ = other.data_;
    error_ = other.error_;
    errorString_ = other.errorString_;
    return *this;
}

template <typename T>
inline ErrorOr<T>& ErrorOr<T>::operator=(ErrorOr<T>&& other)
{
    data_ = std::move(other.data_);
    error_ = other.error_;
    errorString_ = std::move(other.errorString_);
    return *this;
}

template <typename T>
inline ErrorOr<T>::~ErrorOr()
{
}

template <typename T>
inline bool ErrorOr<T>::IsError() const
{
    return error_;
}

template <typename T>
inline T& ErrorOr<T>::Get()
{
    return data_;
}

template <typename T>
inline const T& ErrorOr<T>::Get() const
{
    return data_;
}

template <typename T>
inline std::string ErrorOr<T>::GetError() const
{
    return errorString_;
}

template<>
class ErrorOr<void>
{
    public:
        ErrorOr(std::string errorString);
        ErrorOr();
        ErrorOr(const ErrorOr<void>& other);
        ErrorOr(ErrorOr<void>&& other);
        ErrorOr<void>& operator=(const ErrorOr<void>& other);
        ErrorOr<void>& operator=(ErrorOr<void>&& other);
        ~ErrorOr();

        bool IsError() const;
        std::string GetError() const;
    private:
        bool error_;
        std::string errorString_;
};

inline ErrorOr<void>::ErrorOr(std::string errorString)
    : error_(true), errorString_(errorString)
{
}

inline ErrorOr<void>::ErrorOr()
    : error_(false)
{
}

inline ErrorOr<void>::ErrorOr(const ErrorOr<void>& other)
    : error_(other.error_), errorString_(other.errorString_)
{
}

inline ErrorOr<void>::ErrorOr(ErrorOr<void>&& other)
    : error_(other.error_), errorString_(std::move(other.errorString_))
{
}

inline ErrorOr<void>& ErrorOr<void>::operator=(const ErrorOr<void>& other)
{
    error_ = other.error_;
    errorString_ = other.errorString_;
    return *this;
}

inline ErrorOr<void>& ErrorOr<void>::operator=(ErrorOr<void>&& other)
{
    error_ = other.error_;
    errorString_ = std::move(other.errorString_);
    return *this;
}

inline ErrorOr<void>::~ErrorOr()
{
}

inline bool ErrorOr<void>::IsError() const
{
    return error_;
}

inline std::string ErrorOr<void>::GetError() const
{
    return errorString_;
}
