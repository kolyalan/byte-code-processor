#pragma once

#define CAT(X,Y) X##_##Y
#define TEMPLATE(X,Y) CAT(X,Y)

#define TO_STR(T) #T
#define STR(T) TO_STR(T)
