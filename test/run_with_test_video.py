#!/usr/bin/python

from os import getcwd, system
from os.path import join, isfile
from sys import argv

def main():
    if len(argv) < 2:
        raise SystemExit("no test binary specified")

    test_bin = join(getcwd(), argv[1])

    if not isfile(test_bin):
        raise SystemExit(f"{test_bin} could not be found")

    system(test_bin)


if __name__ == '__main__':
    main()
