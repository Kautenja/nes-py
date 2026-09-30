# nes-py Licensing

The repository's declared software license is the MIT License. The standard
text and Christian Kauten's existing copyright notice are in [LICENSE](LICENSE).
This guide keeps attribution and scope information separate from that text,
following the layout used by [RackNES](https://github.com/Kautenja/RackNES/blob/master/LICENSING.md).
This documentation update does not change the existing license terms.

## License Detection and Package Metadata

Keep the root `LICENSE` limited to the standard license text and copyright
notice. GitHub's [license detection guidance](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository#detecting-a-license)
recommends putting scope explanations elsewhere so its detector can match the
standard text. Put project-specific explanations in this file.

[pyproject.toml](pyproject.toml) and [CITATION.cff](CITATION.cff) retain the
existing SPDX identifier `MIT`. The package configuration includes both
`LICENSE` and `LICENSING.md` in source and wheel distributions. License
detection reports the repository's declaration; it does not establish the
licensing history of every included file.

## Source Attribution and Dependencies

The native emulator derives from [SimpleNES](https://github.com/amhndu/SimpleNES)
by Amish Naidu and contributors, as acknowledged in the README. Preserve
upstream attribution and any file-specific copyright and license notices
when modifying or redistributing derived code.

SimpleNES currently publishes a [GPL version 3 license](https://github.com/amhndu/SimpleNES/blob/master/LICENSE).
That upstream declaration differs from nes-py's existing MIT declaration.
The file layout change documented here does not resolve that provenance
difference or relicense upstream material. Determine the applicable terms
from the imported revisions and any permissions from their copyright holders
before adding or updating third-party code; GitHub's detected license alone
is not evidence of those permissions.

Runtime and build dependencies listed in `pyproject.toml`, and Catch2 fetched
for optional native tests and benchmarks, retain their respective licenses.
The root license does not replace third-party terms. Preserve the relevant
notices and license texts when incorporating or distributing dependency code.

## ROMs, Screenshots, and Names

The project's software license does not grant rights to third-party game
ROMs, game artwork shown in screenshots, or third-party names and trademarks.
Obtain the necessary rights for ROMs used with the emulator, and use original
or redistributable fixtures for new contributions. Identify their source and
terms rather than assuming the repository's license applies to them.

`nes-py` is not affiliated with or approved by Nintendo. The README's
educational-purpose description does not add a use restriction to the MIT
license text.

## Maintaining This Layout

Keep `LICENSE`, the declared SPDX identifiers, and this guide consistent with
any intentional licensing decision. Keep explanatory prose out of `LICENSE`,
preserve existing notices, and verify that distributions include the license
files after packaging changes. Follow [CONTRIBUTING.md](CONTRIBUTING.md) for
the build and contribution workflow.
