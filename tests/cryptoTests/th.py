import os
import glob

class Suite:
    def __init__(self, th):
        hmacSHA256AWSS3 = os.path.abspath(th.path + '/hmac-sha256-aws-s3')
        th.Test('hmacSHA256AWSS3Test', command = [hmacSHA256AWSS3])
