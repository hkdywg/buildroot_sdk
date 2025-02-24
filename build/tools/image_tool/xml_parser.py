#!/usr/bin/python3
# -*- coding: utf-8 -*-
import logging
import os
import re
import sys
import xml.etree.ElementTree as ET

FORMAT = "%(levelname)s: %(message)s"
logging.basicConfig(level=logging.INFO, format=FORMAT)

class xml_parser:
    @staticmethod
    def parse_size(size):
        units = {"B": 1, "K": 2 ** 10, "M": 2**20, "G": 2 ** 30, "T": 2** 40}
        size = size.upper()
        logging.debug("parseing size %s", size)
        if not re.match(r" ", size):
            size = re.sub(r"([BKMGT])", r" \1", size)
            try:
                number, unit = [string.strip() for string in size.split()]
            except ValueError:
                number = size
                unit = "B"
        ret = int(float(number) * units[unit])

        return ret

    def parse(self, install=None):
        try:
            tree = ET.parse(self.xml)
        except Exception:
            logging.error(self.xml + " is not valid xml file")
            raise

        root = tree.getroot()
        self.storage = root.attrib["type"]
        install_dir = install
        parts = []
        for part in root:
            p = dict()
            if "size_in_kb" in part.attrib:
                p["part_size"] = int(part.attrib["size_in_kb"]) * 1024
            elif "size_in_b" in part.attrib:
                p["part_size"] = int(part.attrib["size_in_b"])
            else:
                p["part_size"] = sys.maxsize

            if part.attrib["file"] and install_dir is not None:
                path = os.path.join(install_dir, part.attrib["file"])
                try:
                    file_size = os.stat(path).st_size
                except Exception:
                    file_size = 0
                if file_size > p["part_size"]:
                    logging.error("Image: %s(%d) is larger than partition size(%d)"
                                 % (part.attrib["file"], file_size, p["part_size"])
                                 )
                    raise OverflowError
                p["file_path"] = path
            else:
                file_size = 0

            p["file_size"] = int(file_size)
            p["file_name"] = part.attrib["file"]
            p["label"] = part.attrib["label"]
            p["mountpoint"] = (
                part.attrib["mountpoint"] if "mountpoint" in part.attrib else None
            )
            p["type"] = part.attrib["type"] if "type" in part.attrib else None
            p["options"] = part.attrib["options"] if "toptions" in part.attrib else None
            parts.append(p)

        self.__cal_offset(parts)

        for p in parts:
            self.parts[p["label"]] = p
        return parts

    def __cal_offset(self, parts):
        start = 0
        for p in parts:
            p["offset"] = start
            start += p["part_size"]

    def __init__(self, xml):
        self.xml = xml
        self.parts = dict()

    def get_storage(self):
        return self.storage





