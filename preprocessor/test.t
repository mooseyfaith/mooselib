
def B struct {
	var bla: A;
	var x: *B;

	kind y: u32;
	kind z: struct {
		var a: *A;
		var b: *B;
	}

	kind as: A;
}

def A struct {
	var fun: string;
	var sadness: s64;
	var soul: f32;
}

def div_mod function(a: u32, b: u32) -> (u32, u32) {
	return a / b, a % b;
}

def main function(argument_count: s32, arguments: **u8) -> (s32) {

	var win32_api: Win32_Platform_API;
    init_win32_api(&win32_api);

    var platform_api: *Platform_API = &win32_api.platform_api;
    init_c_allocator();
    init_Memory_Growing_Stack_allocators();

	var c: s32 = 12;

	printf("hello world\n");

	printf("A c: %u, t: %u\n", (u32)sizeof(A), (u32)A::Byte_Count);
	printf("B c: %u, t: %u\n", (u32)sizeof(B), (u32)B::Byte_Count);

	var b: B = {};
	b = make_kind(B, y, 12);
	printf("B.kind: %u (%u)\n", (u32)b.kind, (u32)b.Byte_Count);

	auto y = kind_of(&b, y);
	printf("B as y: %u\n", *y);

	var transient_memory: Memory_Growing_Stack = make_memory_growing_stack(platform_api->allocator);
	var transient_allocator: *Memory_Allocator = &transient_memory.allocator;

	var x: s32 = run &transient_memory, fun(S("fuuck"), 12);
}

def fun coroutine(text: string, count: u32) -> (s32){
	var i: u32 = 0;

	while i < count do {
		printf("%.*s\n", FORMAT_S(&text));
		yield 0;
		i = i + 1;
	}
}