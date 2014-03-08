class Suite:
    def __init__(self, th):
        self.th = th

    def build(self): self.th.system('scons')
    def clean(self): self.th.system(['scons', '-c'])
