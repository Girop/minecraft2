#pragma once 

#define GETTER(type, field) \
    type& field() { \
        return field##_; \
    } \
    type const& field() const {\
        return field##_;\
    }

