import os

class Suite:
    def __init__(self, th):
	cmd = os.path.abspath(th.path + '/iprange')

        th.Test('All', command = [cmd, '0/0'])
        th.Test('CIDR', command = [cmd, '0.0.0.0/24'])
        th.Test('Inside', command = [cmd, '0.0.0.0/24', '0.0.0.1'])
        th.Test('LeftEdge', command = [cmd, '0.0.0.0/24', '0.0.0.0'])
        th.Test('RightEdge', command = [cmd, '0.0.0.0/24', '0.0.0.255'])
        th.Test('Less', command = [cmd, '0.0.0.1', '0.0.0.0'])
        th.Test('Greater', command = [cmd, '0.0.0.1', '0.0.0.2'])
        th.Test('Between', command = [cmd, '0.0.0.1 0.0.0.3', '0.0.0.2'])

        ranges = '0.0.0.0/24 0.0.1.0/24 0.0.2.0/24 0.0.3.0/24 0.0.4.0/24'
        th.Test('CollapseCenter', command = [cmd,  ranges + ' 0.0.1.0-0.0.3.0'])
        th.Test('CollapseLeft', command = [cmd,  ranges + ' 0.0.0.255-0.0.3.0'])
        th.Test('CollapseRight', command = [cmd,  ranges + ' 0.0.1.0-0.0.7.0'])

        th.Test('Backwards', command = [cmd, '0.0.0.1-0.0.0.0'])

        ranges = '166.204.5.157 102.30.149.17 241.87.234.3 68.102.71.218 ' \
            '116.236.180.179 22.241.170.84 99.89.219.75 31.99.102.141 ' \
            '82.142.250.28 91.247.141.197'
        th.Test('Order', command = [cmd, ranges])

        ranges = '116.194.187.138-95.27.34.183 212.136.158.34-86.238.250.231 ' \
            '23.222.72.151-20.186.241.160 3.115.103.80-4.220.116.86 ' \
            '60.205.123.9-102.22.48.218 198.38.18.105-255.109.145.147 ' \
            '54.178.10.51-27.114.2.169 232.25.231.150-198.118.55.198 ' \
            '190.136.6.96-75.76.242.2 12.243.152.71-12.67.98.133 '
        th.Test('Random', command = [cmd, ranges])
