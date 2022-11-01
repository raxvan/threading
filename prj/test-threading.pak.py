
def configure(cfg):
	t = cfg.link("../../ttf/testing.pak.py")
	cfg.link("../../nstimer/prj/nstimer.pak.py")

	t.enable()
	cfg.link("threading.pak.py")


def construct(ctx):
	
	ctx.config("type","exe")

	ctx.fscan("src: ../test")


