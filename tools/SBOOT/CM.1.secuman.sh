#!/bin/bash
export LANG=C
TOOLNAME=./certtool/da16secutool
# TOOLNAME=py -3.8 .\certtool_py\da16secutool.py

${TOOLNAME} -mod secuman -nofix -cfg ./cmconfig/da16xtpmconfig.cfg
