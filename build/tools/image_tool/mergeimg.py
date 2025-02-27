#!/usr/bin/python3
# -*- coding: utf-8 -*-

import logging
import argparse
import argparse
import sys
import os
from xml_parser import xml_parser
import subprocess

FORMAT = "%(levelname)s: %(message)s"
logging.basicConfig(level=logging.INFO, format=FORMAT)

def gen_system_img(parts, img_path):
    lba_index = 2048
    lba_size = 512

    command = f"dd if={img_path}/main_gpt.emmc of={img_path}/burn_system.emmc bs=512"
    subprocess.call(command, shell=True, text=True)

    for p in parts:
        start = lba_index
        end = start + ((p["part_size"] + lba_size -1) / lba_size) - 2
        img_name = p["file_name"]
        seek_index = int(lba_index)
        if p["file_name"]:
            command = f"dd if={img_path}/{img_name} of={img_path}/burn_system.emmc bs=512 seek={seek_index}"
            subprocess.call(command, shell=True, text=True)
            logging.info("commad: %s" % command)
        lba_index = end + 1


def parse_args():
    parser = argparse.ArgumentParser(description="Create the system image, it can be burn to emmc(include gpt partition)")

    parser.add_argument(
        "xml",
        metavar="xml",
        type=str,
        help="the partition configure file(xml) about image"
    )
    parser.add_argument(
        "image_path",
        metavar="image_path",
        type=str,
        help="the path to install, include main and back partition image"
    )
    parser.add_argument("-v", "--verbose", help="increase output verbosity", action="store_true")
    args = parser.parse_args()
    if(args.verbose):
        logging.debug("Enable more verbose output")
        logging.getLogger().setLevel(level=logging.DEBUG)

    return args


if __name__ == '__main__':
    args = parse_args()
    xmlparser = xml_parser(args.xml)
    parts = xmlparser.parse()
    gen_system_img(parts, args.image_path)


