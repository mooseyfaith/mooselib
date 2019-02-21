

def A struct {
	var fun: string;
	var sadness: s64;
	var soul: f32;
}

def B struct {
	var bla: A;
	var x: *B;

	kind y: u32;
	kind z: struct {
		var a: *A;
		var b: *B;
	}
}

def main function(argument_count: s32, arguments: **u8) -> (s32) {
	printf("hello world\n");

	printf("A c: %u, t: %u\n", (u32)sizeof(A), (u32)A::Byte_Count);
	printf("B c: %u, t: %u\n", (u32)sizeof(B), (u32)B::Byte_Count);

	var b: B = {};
	b = make_kind(B, y, 12);
	b.y = 13;
	printf("B.kind: %u\n", (u32)b.kind);

	auto y = kind_of(&b, y);
	printf("B as y: %u\n", *y);
}
