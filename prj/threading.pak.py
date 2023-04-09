
def configure(cfg):
	cfg.link_if_enabled("../../ttf/testing.pak.py")

	cfg.link("../../dev-platform/prj/dev-platform.pak.py")
	#cfg.link("../../srcgen/cppreflect/prj/reflection.pak.py")

def construct(ctx):
	ctx.config("type","lib")

	ctx.folder("public include: ../incl")

	ctx.fscan("src: ../incl")
	ctx.fscan("src: ../src")

	if ctx.module_enabled("testing"):
		ctx.assign("public define:THREADING_TESTING")

