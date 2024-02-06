#!/bin/bash

set -e

if [ ! -d build ]; then
  echo "Please run this script from the root folder"
  exit
fi

ROOT="$(pwd)"

if [ -e build/mod-desktop.app ]; then
  HTML_DIR="build/mod-desktop.app/Contents/Resources/html"
else
  HTML_DIR="build/html"
fi

rm -rf "${HTML_DIR}"
mkdir "${HTML_DIR}"
cd "${HTML_DIR}"

cp ${ROOT}/src/mod-ui/html/*.html ./
cp ${ROOT}/src/mod-ui/html/favicon.ico ./

mkdir ./css
cp ${ROOT}/src/mod-ui/html/css/*.css ./css/

mkdir ./css/fontello
mkdir ./css/fontello/css
cp ${ROOT}/src/mod-ui/html/css/fontello/css/*.css ./css/fontello/css/

mkdir ./css/fontello/font
cp ${ROOT}/src/mod-ui/html/css/fontello/font/*.eot ./css/fontello/font/
cp ${ROOT}/src/mod-ui/html/css/fontello/font/*.svg ./css/fontello/font/
cp ${ROOT}/src/mod-ui/html/css/fontello/font/*.ttf ./css/fontello/font/
cp ${ROOT}/src/mod-ui/html/css/fontello/font/*.woff ./css/fontello/font/
cp ${ROOT}/src/mod-ui/html/css/fontello/font/*.woff2 ./css/fontello/font/

mkdir ./fonts
mkdir ./fonts/Ek-Mukta
mkdir ./fonts/Ek-Mukta/Ek-Mukta-200
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-200/*.eot ./fonts/Ek-Mukta/Ek-Mukta-200/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-200/*.svg ./fonts/Ek-Mukta/Ek-Mukta-200/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-200/*.ttf ./fonts/Ek-Mukta/Ek-Mukta-200/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-200/*.woff ./fonts/Ek-Mukta/Ek-Mukta-200/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-200/*.woff2 ./fonts/Ek-Mukta/Ek-Mukta-200/

mkdir ./fonts/Ek-Mukta/Ek-Mukta-600
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-600/*.eot ./fonts/Ek-Mukta/Ek-Mukta-600/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-600/*.svg ./fonts/Ek-Mukta/Ek-Mukta-600/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-600/*.ttf ./fonts/Ek-Mukta/Ek-Mukta-600/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-600/*.woff ./fonts/Ek-Mukta/Ek-Mukta-600/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-600/*.woff2 ./fonts/Ek-Mukta/Ek-Mukta-600/

mkdir ./fonts/Ek-Mukta/Ek-Mukta-700
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-700/*.eot ./fonts/Ek-Mukta/Ek-Mukta-700/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-700/*.svg ./fonts/Ek-Mukta/Ek-Mukta-700/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-700/*.ttf ./fonts/Ek-Mukta/Ek-Mukta-700/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-700/*.woff ./fonts/Ek-Mukta/Ek-Mukta-700/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-700/*.woff2 ./fonts/Ek-Mukta/Ek-Mukta-700/

mkdir ./fonts/Ek-Mukta/Ek-Mukta-regular
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-regular/*.eot ./fonts/Ek-Mukta/Ek-Mukta-regular/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-regular/*.svg ./fonts/Ek-Mukta/Ek-Mukta-regular/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-regular/*.ttf ./fonts/Ek-Mukta/Ek-Mukta-regular/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-regular/*.woff ./fonts/Ek-Mukta/Ek-Mukta-regular/
cp ${ROOT}/src/mod-ui/html/fonts/Ek-Mukta/Ek-Mukta-regular/*.woff2 ./fonts/Ek-Mukta/Ek-Mukta-regular/

mkdir ./fonts/comforta
cp ${ROOT}/src/mod-ui/html/fonts/comforta/*.ttf ./fonts/comforta/

mkdir ./fonts/cooper
cp ${ROOT}/src/mod-ui/html/fonts/cooper/*.eot ./fonts/cooper/
cp ${ROOT}/src/mod-ui/html/fonts/cooper/*.ttf ./fonts/cooper/
cp ${ROOT}/src/mod-ui/html/fonts/cooper/*.woff ./fonts/cooper/
cp ${ROOT}/src/mod-ui/html/fonts/cooper/*.woff2 ./fonts/cooper/

mkdir ./fonts/css
cp ${ROOT}/src/mod-ui/html/fonts/css/*.css ./fonts/css/

mkdir ./fonts/england-hand
cp ${ROOT}/src/mod-ui/html/fonts/england-hand/*.css ./fonts/england-hand/
cp ${ROOT}/src/mod-ui/html/fonts/england-hand/*.eot ./fonts/england-hand/
cp ${ROOT}/src/mod-ui/html/fonts/england-hand/*.svg ./fonts/england-hand/
cp ${ROOT}/src/mod-ui/html/fonts/england-hand/*.ttf ./fonts/england-hand/
cp ${ROOT}/src/mod-ui/html/fonts/england-hand/*.woff ./fonts/england-hand/

mkdir ./fonts/epf
cp ${ROOT}/src/mod-ui/html/fonts/epf/*.css ./fonts/epf/
cp ${ROOT}/src/mod-ui/html/fonts/epf/*.eot ./fonts/epf/
cp ${ROOT}/src/mod-ui/html/fonts/epf/*.svg ./fonts/epf/
cp ${ROOT}/src/mod-ui/html/fonts/epf/*.ttf ./fonts/epf/
cp ${ROOT}/src/mod-ui/html/fonts/epf/*.woff ./fonts/epf/

mkdir ./fonts/nexa
cp ${ROOT}/src/mod-ui/html/fonts/nexa/*.css ./fonts/nexa/
cp ${ROOT}/src/mod-ui/html/fonts/nexa/*.eot ./fonts/nexa/
cp ${ROOT}/src/mod-ui/html/fonts/nexa/*.svg ./fonts/nexa/
cp ${ROOT}/src/mod-ui/html/fonts/nexa/*.ttf ./fonts/nexa/
cp ${ROOT}/src/mod-ui/html/fonts/nexa/*.woff ./fonts/nexa/

mkdir ./fonts/pirulen
cp ${ROOT}/src/mod-ui/html/fonts/pirulen/*.css ./fonts/pirulen/
cp ${ROOT}/src/mod-ui/html/fonts/pirulen/*.eot ./fonts/pirulen/
cp ${ROOT}/src/mod-ui/html/fonts/pirulen/*.ttf ./fonts/pirulen/
cp ${ROOT}/src/mod-ui/html/fonts/pirulen/*.woff ./fonts/pirulen/

mkdir ./fonts/questrial
cp ${ROOT}/src/mod-ui/html/fonts/questrial/*.css ./fonts/questrial/
cp ${ROOT}/src/mod-ui/html/fonts/questrial/*.eot ./fonts/questrial/
cp ${ROOT}/src/mod-ui/html/fonts/questrial/*.svg ./fonts/questrial/
cp ${ROOT}/src/mod-ui/html/fonts/questrial/*.ttf ./fonts/questrial/
cp ${ROOT}/src/mod-ui/html/fonts/questrial/*.woff ./fonts/questrial/

mkdir ./img
cp ${ROOT}/src/mod-ui/html/img/*.gif ./img/
cp ${ROOT}/src/mod-ui/html/img/*.jpg ./img/
cp ${ROOT}/src/mod-ui/html/img/*.png ./img/
cp ${ROOT}/src/mod-ui/html/img/*.svg ./img/

mkdir ./img/cloud
cp ${ROOT}/src/mod-ui/html/img/cloud/*.png ./img/cloud/

mkdir ./img/favicon
cp ${ROOT}/src/mod-ui/html/img/favicon/*.png ./img/favicon/

mkdir ./img/icons
cp ${ROOT}/src/mod-ui/html/img/icons/*.css ./img/icons/
cp ${ROOT}/src/mod-ui/html/img/icons/*.svg ./img/icons/
cp ${ROOT}/src/mod-ui/html/img/icons/*.png ./img/icons/

mkdir ./img/icons/25
cp ${ROOT}/src/mod-ui/html/img/icons/25/*.png ./img/icons/25/

mkdir ./img/icons/36
cp ${ROOT}/src/mod-ui/html/img/icons/36/*.png ./img/icons/36/

mkdir ./img/social
cp ${ROOT}/src/mod-ui/html/img/social/*.png ./img/social/

mkdir ./include
cp ${ROOT}/src/mod-ui/html/include/*.html ./include/

mkdir ./js
cp ${ROOT}/src/mod-ui/html/js/*.js ./js/

mkdir ./js/utils
cp ${ROOT}/src/mod-ui/html/js/utils/*.js ./js/utils/

mkdir ./js/lib
cp ${ROOT}/src/mod-ui/html/js/lib/*.js ./js/lib/

mkdir ./js/lib/slick
cp ${ROOT}/src/mod-ui/html/js/lib/slick/*.js ./js/lib/slick/
cp ${ROOT}/src/mod-ui/html/js/lib/slick/*.css ./js/lib/slick/
cp ${ROOT}/src/mod-ui/html/js/lib/slick/*.gif ./js/lib/slick/

mkdir ./js/lib/slick/fonts
cp ${ROOT}/src/mod-ui/html/js/lib/slick/fonts/*.eot ./js/lib/slick/fonts/
cp ${ROOT}/src/mod-ui/html/js/lib/slick/fonts/*.svg ./js/lib/slick/fonts/
cp ${ROOT}/src/mod-ui/html/js/lib/slick/fonts/*.ttf ./js/lib/slick/fonts/
cp ${ROOT}/src/mod-ui/html/js/lib/slick/fonts/*.woff ./js/lib/slick/fonts/

mkdir ./resources
cp ${ROOT}/src/mod-ui/html/resources/*.html ./resources/

mkdir ./resources/pedals
cp ${ROOT}/src/mod-ui/html/resources/pedals/*.css ./resources/pedals/
cp ${ROOT}/src/mod-ui/html/resources/pedals/*.png ./resources/pedals/

mkdir ./resources/templates
cp ${ROOT}/src/mod-ui/html/resources/templates/*.html ./resources/templates/
