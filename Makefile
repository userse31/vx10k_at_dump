all:
	gcc dump.c -o dump
	gcc dump_multitry.c -o dump_multitry
	gcc patch_vx10k.c -o patch_vx10k
	gcc poke_vx10k.c -o poke_vx10k
