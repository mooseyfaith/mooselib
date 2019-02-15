#include "preprocessor_coroutines.h"

u32 factorial(u32 n);
s32 div(s32 *_out1, s32 a, s32 b);
void test();
COROUTINE_DEC(factorial_recursive);
COROUTINE_DEC(factorial_iterative);

u32 factorial(u32 n) {
if (n <= 1 ) {
return 1;
}

return factorial(n - 1) * n;
}



s32 div(s32 *_out1, s32 a, s32 b) {
*_out1 = a % b;
return a / b;
}



void test() {
s32 x;
s32  y;
x = div(&(y), 123, 3);
u32  z;
z = factorial(3);
}



COROUTINE_DEC(factorial_recursive) {
switch (CO_STATE_ENTRY) {
case 0:
goto co_label0;

case 1:
goto co_label1;

default:
assert(0);
}

// argument: def n: u32;
co_label0:

if (CO_ENTRY(u32, 12) <= 1 ) {

{ // return
u32 return0 = 1;
CO_POP_COROUTINE;
CO_PUSH(u32) = return0;
return Coroutine_Continue;
}

}

CO_PUSH(u32 ); // local variable p

{ // call factorial_recursive
CO_STATE_ENTRY = 1;
u32 arg0 = CO_ENTRY(u32, 12) - 1;
CO_PUSH_COROUTINE(factorial_recursive);
CO_PUSH(u32) = arg0;
return Coroutine_Continue;
}
co_label1:

{ // read results
CO_ENTRY(u32 , 16) = CO_ENTRY(u32, 20);
CO_POP(u32); // pop return value 0
}


{ // return
u32 return0 = CO_ENTRY(u32, 12) * CO_ENTRY(u32 , 16);
CO_POP_COROUTINE;
CO_PUSH(u32) = return0;
return Coroutine_Continue;
}

CO_POP(u32 ); // local variable p 
CO_POP(u32); // local variable n 
}



COROUTINE_DEC(factorial_iterative) {
switch (CO_STATE_ENTRY) {
case 0:
goto co_label0;

case 1:
goto co_label1;

default:
assert(0);
}

// argument: def n: u32;
co_label0:

CO_PUSH(u32 ); // local variable a
CO_ENTRY(u32 , 16) = 1;
CO_PUSH(u32 ); // local variable f
CO_ENTRY(u32 , 20) = CO_ENTRY(u32 , 16);
while (CO_ENTRY(u32 , 16) < CO_ENTRY(u32, 12) ) {

{ // yield
u32 return0 = CO_ENTRY(u32 , 20);
CO_PUSH(u32) = return0;
CO_STATE_ENTRY = 1;
return Coroutine_Wait;
}

co_label1:

CO_ENTRY(u32 , 16)++;
CO_ENTRY(u32 , 20) *= CO_ENTRY(u32 , 16);
}


{ // return
u32 return0 = CO_ENTRY(u32 , 20);
CO_POP_COROUTINE;
CO_PUSH(u32) = return0;
return Coroutine_Continue;
}

CO_POP(u32 ); // local variable f 
CO_POP(u32 ); // local variable a 
CO_POP(u32); // local variable n 
}



