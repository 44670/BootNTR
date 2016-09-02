; setup constants
	.const c20, 1.0, 1.0, 1.0, 1.0

; setup outmap
	.out o0, result.position, 0xF
	.out o1, result.color, 0xF
	.out o2, result.texcoord0, 0x3

; setup uniform map (not required)
	.uniform c0, c3, projection
	.uniform c4, c7, modelview

	.vsh vmain, end_vmain

; code
	vmain:
		; temp = modelview * in.pos
		dp4 r0, c4, v0 (0x0)
		dp4 r0, c5, v0 (0x1)
		dp4 r0, c6, v0 (0x2)
		mov r0, c20 (0x3)
		; result.pos = projection * temp
		dp4 o0, c0, r0 (0x0)
		dp4 o0, c1, r0 (0x1)
		dp4 o0, c2, r0 (0x2)
		dp4 o0, c3, r0 (0x3)
		; result.color = in.color
		mov o1, v2 (0x5)
		; result.texcoord = in.texcoord
		mov o2, v1 (0x5)
		; end
		nop
		end
	end_vmain:

; operand descriptors
	.opdesc x___, xyzw, xyzw ; 0x0
	.opdesc _y__, xyzw, xyzw ; 0x1
	.opdesc __z_, xyzw, xyzw ; 0x2
	.opdesc ___w, xyzw, xyzw ; 0x3
	.opdesc xyz_, xyzw, xyzw ; 0x4
	.opdesc xyzw, xyzw, xyzw ; 0x5
	.opdesc x_zw, xyzw, xyzw ; 0x6
	.opdesc xyzw, yyyw, xyzw ; 0x7
	.opdesc xyz_, wwww, wwww ; 0x8
	.opdesc xyz_, yyyy, xyzw ; 0x9
