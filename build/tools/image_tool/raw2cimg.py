#!/usr/bin/python3
# -*- coding: utf-8 -*-
import logging
import argparse
import os
from array import array
import binascii
from xml_parser import xml_parser
from tempfile import TemporaryDirectory
import shutil

MAX_LOAD_SIZE = 16 * 1024 * 1024
CHUNK_TYPE_DONT_CARE = 0
CHUNK_TYPE_CRC_CHECK = 1
FORMAT = "%(levelname)s: %(message)s"
logging.basicConfig(level=logging.INFO, format=FORMAT)

def parse_args():
    parser = argparse.ArgumentParser(description="Create the device image")

    parser.add_argument(
        "file_path",
        metavar="file_path",
        type=str,
        help="the file you want to pack with image header"
    )
    parser.add_argument(
        "output_path",
        metavar="output_path",
        type=str,
        help="the path to install, include rootfs and kernel"
    )
    parser.add_argument("xml", help="path to partition xml")
    parser.add_argument("-v", "--verbose", help="increase output verbosity", action="store_true")
    args = parser.parse_args()
    if(args.verbose):
        logging.debug("Enable more verbose output")
        logging.getLogger().setLevel(level=logging.DEBUG)

    return args


class image_builder(object):
    def __init__(self, storage: int, output_path):
        self.storage = storage
        self.output_path = output_path

    def pack_header(self, part):
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

        with open(part["file_path"], "rb") as fd:
            magic = fd.read(4)
            if magic == b"CIMG":
                logging.debug("%s has been packged, skip it!" % part["file_name"])
                return
            fd.seek(0)
            magic = array("b", [ord(c) for c in "CIMG"])
            version = array("I", [1])
            chunk_header_size = 64
            chunk_size = array("I", [chunk_header_size])
            chunk_counts = part["file_size"] // MAX_LOAD_SIZE
            remain = part["file_size"] - MAX_LOAD_SIZE * chunk_counts
            if(remain != 0):
                chunk_counts = chunk_counts + 1
            total_chunk = array("I", [chunk_counts])
            file_size = array("I", [part["file_size"] + (chunk_counts * chunk_header_size)])
            try:
                label = part["label"]
            except KeyError:
                label = "gpt"
            extra_flag = array("B", [ord(c) for c in label])
            for _ in range(len(label), 32):
                extra_flag.append(ord("\0"))

            img = open(os.path.join(self.output_path, part["file_name"]), "wb")
            # write header
            for h in [magic, version, chunk_size, total_chunk, file_size, extra_flag]:
                h.tofile(img)
            img.seek(64)
            total_size = part["file_size"]
            offset = part["offset"]
            part_size = part["part_size"]
            op_len = 0
            while total_size:
                write_chunk_size = min(MAX_LOAD_SIZE, total_size)
                chunk = fd.read(write_chunk_size)
                crc = binascii.crc32(chunk) & 0xFFFFFFFF
                if write_chunk_size == MAX_LOAD_SIZE:
                    op_len += MAX_LOAD_SIZE
                else:
                    op_len = part_size - op_len
                chunk_header = self._get_chunk_header(write_chunk_size, offset, op_len, crc)
                img.write(chunk_header)
                img.write(chunk)
                total_size -= write_chunk_size
                offset += write_chunk_size
            img.close()

    def _get_chunk_header(self, size: int, offset: int, part_size: int, crc32: int):
        """
        Header format total 64 bytes
        4 Bytes: chunk type
        4 Bytes: chunk data size
        4 Bytes: program part offest
        4 Bytes: program part size
        4 Bytes: crc32 checksum
        """
        logging.info("size:%x, offset:%x, part_size:%x, crc:%x" % (size, offset, part_size, crc32))
        chunk = array(
            "I",
            [
                CHUNK_TYPE_CRC_CHECK,
                size,
                offset,
                part_size,
                crc32,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
            ],
        )
        return chunk
            
            
def main():
    args = parse_args()
    xmlparser = xml_parser(args.xml)
    install_dir = os.path.dirname(args.file_path)
    parts = xmlparser.parse(install_dir)
    storage = xmlparser.get_storage()
    tmp = TemporaryDirectory()
    imgbuilder = image_builder(storage, tmp.name)
    #imgbuilder = image_builder(storage, args.output_path)
    for p in parts:
        if os.path.basename(args.file_path) == p["file_name"]:
            tmp_path = os.path.join(tmp.name, p["file_name"])
            imgbuilder.pack_header(p)
            out_path = os.path.join(args.output_path, p["file_name"])
            logging.info("moving %s -> %s" % (tmp_path, out_path))
            shutil.move(tmp_path, out_path)
            logging.info("packinng %s done!" % (p["file_name"]))


if __name__ == "__main__":
    main()
