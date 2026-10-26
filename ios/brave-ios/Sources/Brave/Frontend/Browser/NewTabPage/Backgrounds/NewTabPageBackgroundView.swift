// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import SnapKit
import UIKit

/// Non-interactive contents that appear behind the New Tab Page contents
class NewTabPageBackgroundView: UIView {
  /// The image wallpaper if the user has background images enabled
  let imageView = UIImageView().then {
    $0.contentMode = .scaleAspectFill
    $0.clipsToBounds = false
  }

  func updateImageXOffset(by x: CGFloat) {
    bounds = .init(x: -x, y: 0, width: bounds.width, height: bounds.height)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    clipsToBounds = true
    // Growser-392: with background images off the page is a flat theme colour, the
    // same as the desktop's; the tiles follow the theme (NewTabPageViewController).
    backgroundColor = .init {
      $0.userInterfaceStyle == .dark ? .init(rgb: 0x202124) : .init(rgb: 0xf4f4f5)  // Growser-392
    }

    addSubview(imageView)
    imageView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
