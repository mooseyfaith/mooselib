
def B struct {
	def X struct { var z: Z; }

	var x: *B;

	def Z struct {
		var a: As;
		var b: *B;
	}

	kind y: struct { 
		var x: X;
	}, z: Z;
}

def As struct { var a: A; }

def A struct {

	var fun: string;
	var sadness: s64;
	var soul: f32;
}

def div_mod function(a: u32, b: u32) -> (u32, u32) {
	return a / b, a % b;
}

def main function(argument_count: s32, arguments: **u8) -> (s32) {

	def foo function() {
		printf("foo!");
	}

	var win32_api: Win32_Platform_API;
    init_win32_api(&win32_api);

    var platform_api: *Platform_API = &win32_api.platform_api;
    init_c_allocator();
    init_Memory_Growing_Stack_allocators();

	printf("hello world\n");

	printf("A c: %u, t: todo\n", (u32)sizeof(A));
	printf("B c: %u, t: todo\n", (u32)sizeof(B));

	var b: B = {};
	b = make_kind(B, y);

	auto y = kind_of(&b, y);
	printf("B as y: %p\n", y);

	var transient_memory: Memory_Growing_Stack = make_memory_growing_stack(platform_api->allocator);
	var transient_allocator: *Memory_Allocator = &transient_memory.allocator;

}

def fun coroutine(text: string, count: u32) -> (s32) {
	var i: u32 = 0;

	while i < count do fun {
		continue fun;
		printf("%.*s\n", FORMAT_S(&text));
		yield 0;
		i = i + 1;
	}
}
