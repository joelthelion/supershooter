# Contributor: elcerdo <georges.wbush@laposte.net>
maintainer=elcerdo
pkgname=supershooter
pkgver=0.3.1
pkgrel=1
pkgdesc="a shoot them up game with a lot of bullets"
arch=(i686 x86_64)
url="http://github.com/elcerdo/supershooter"
license=('GPL')
depends=(sdl sdl_mixer sdl_image boost ode)
makedepends=(cmake)
source=(http://github.com/elcerdo/${pkgname}/tarball/${pkgver})
md5sums=('618098cb2c62a4613842d519b1503038')

build() {
  cd "${srcdir}/$(ls ${srcdir} | grep "elcerdo")"

  cmake . \
  -DCMAKE_INSTALL_PREFIX:FILEPATH=/usr \
  -DCMAKE_BUILD_TYPE:STRING=Release

  make || return 1
  make DESTDIR="$pkgdir/" install
}

# vim:set ts=2 sw=2 et:
