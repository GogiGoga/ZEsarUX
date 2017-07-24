#!/bin/bash

#eliminar - del texto
TEXTO=`cat|sed 's/-//'`

say  "$TEXTO"
