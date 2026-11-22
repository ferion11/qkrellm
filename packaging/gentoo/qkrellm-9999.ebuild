# Copyright 2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit cmake git-r3 xdg

DESCRIPTION="Lightweight Qt6 system monitor in the spirit of GKrellM"
HOMEPAGE="https://github.com/ferion11/qkrellm"
EGIT_REPO_URI="https://github.com/ferion11/qkrellm.git"

LICENSE="GPL-3+"
SLOT="0"
IUSE="test"
RESTRICT="!test? ( test )"

DEPEND="dev-qt/qtbase:6[gui,widgets]"
RDEPEND="${DEPEND}"

src_configure() {
	local mycmakeargs=(
		-DQKRELLM_BUILD_TESTS=$(usex test)
		-DQKRELLM_USE_CCACHE=OFF
		-DQKRELLM_USE_MOLD=OFF
	)
	cmake_src_configure
}

src_test() {
	QT_QPA_PLATFORM=offscreen cmake_src_test
}
