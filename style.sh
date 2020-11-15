#!/bin/bash

# astyle -A2s4SxWwYm0M120pxgHk3W3xbjxfxhcxyxC120xLnrvz2 ./src/*.cpp,*.h

astyle \
--style=attach \
--indent=spaces=4 \
--indent-switches \
--indent-preproc-block \
--indent-preproc-define \
--indent-col1-comments \
--min-conditional-indent=0 \
--max-continuation-indent=120\
--pad-oper \
--pad-comma \
--pad-header \
--align-pointer=name \
--align-reference=name \
--break-one-line-headers \
--add-braces \
--attach-return-type \
--attach-return-type-decl \
--convert-tabs \
--close-templates \
--max-code-length=120 \
--break-after-logical \
--suffix=none \
--recursive \
--verbose \
--lineend=linux \
./src/*.cpp,*.h
