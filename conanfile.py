#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from conans import ConanFile, tools

def get_version():
    git = tools.Git()
    try:
        return git.run("describe --tags --abbrev=0")
    except:
        return None

class ContinuableConan(ConanFile):
    name = "continuable"
    version = get_version()
    license = "MIT"
    url = "https://github.com/Naios/continuable"
    author = "Denis Blank (denis.blank@outlook.com)"
    description = "C++14 asynchronous allocation aware futures"
    homepage = "https://naios.github.io/continuable/"
    no_copy_source = True
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    def package(self):
        self.copy("LICENSE.txt", "licenses")
        self.copy("include/*.hpp")
        self.copy("include/*.inl")

    def package_id(self):
        self.info.header_only()

    def requirements(self):
        self.requires("function2/4.0.0@naios/stable")
