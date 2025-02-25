#!/usr/bin/python3
# -*- coding: utf-8 -*-
import logging
import argparse
import errno
from os import getcwd, path, makedirs

MAX_LOAD_SIZE = 100 *  1024 * 1024
CHUNK_TYPE_DONT_CARE = 0
CHUNK_TYPE_CRC_CHECK = 1
FORMAT = "%(levelname)s: %(message)s"
logging.basicConfig(level=logging.INFO, format=FORMAT)

def parse_args():
    parser = argparse.ArgumentParser(description="Create device image")

    parser.add_argument(
        "file_path",
        metavar="file_path",
        type=str,
        help="the file you want to pack witch image header",
    )

    parser.add_argument(
        "--output_dir",
        metavar="output_folder",
        type=str,
        help="the folder to save output file, default whill be ./rawimages",
        default=path.join(getcwd(), "rawimages"),
    )

    parser.add_argument("-v", "--verbose", help="increase output verbosity", action="store_true")
    args = parser.parse_args()
    if(args.verbose):
        logging.debug("Enable more verbose output")
        logging.getLogger().setLevel(level=logging.DEBUG)

    return args

class imager_remover(object):
    @staticmethod
    def  remove_header(img, out):
        """
        Header format total 64 bytes
        4 Bytes: Magic
        4 Bytes: Version
        4 Bytes: Chunk header size
        4 Bytes: Total chunks 
        4 Bytes: File size
        32 Bytes: Extra Flags
        12 Bytes: Reserved
        """
        with open(img, "rb") as fd:
            magic = fd.read(4)
            if magic != b"CIMG":
                logging.error("%s is not iamge!" % img)
                raise  TypeError

            with open(out, "wb") as fo:
                fd.seek(64) # skip header
                # skip chunk header
                while fd.read(64):
                    fo.write(fd.read(MAX_LOAD_SIZE))

def main():
    args = parse_args()
    output_path = path.join(args.output_dir, path.basename(args.file_path))
    try:
        makedirs(args.output_dir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise
    imager_remover.remove_header(args.file_path, output_path)
    logging.info("Write %s Done" % output_path)

if __name__ == "__main__":
    main()

