#!/usr/bin/env sh
set -eu

root="${1:-}"
prefix="${2:-/usr}"

if [ -z "${root}" ]; then
    echo "Usage: $0 STAGING_ROOT [PREFIX]" >&2
    exit 2
fi

case "${prefix}" in
    /*) ;;
    *) echo "PREFIX must be absolute: ${prefix}" >&2; exit 2 ;;
esac

install_root="${root}${prefix}"
required_files="
${install_root}/bin/linscreen
${install_root}/share/applications/org.linscreen.LinScreen.desktop
${install_root}/share/metainfo/org.linscreen.LinScreen.metainfo.xml
${install_root}/share/dbus-1/interfaces/org.linscreen.LinScreen.xml
${install_root}/share/dbus-1/services/org.linscreen.LinScreen.service
${install_root}/share/linscreen/translations/Internationalization_fr.qm
${install_root}/share/icons/hicolor/scalable/apps/org.linscreen.LinScreen.svg
"

echo "${required_files}" | while IFS= read -r file; do
    [ -z "${file}" ] && continue
    if [ ! -s "${file}" ]; then
        echo "Missing or empty installed file: ${file}" >&2
        exit 1
    fi
done

desktop="${install_root}/share/applications/org.linscreen.LinScreen.desktop"
service="${install_root}/share/dbus-1/services/org.linscreen.LinScreen.service"
metainfo="${install_root}/share/metainfo/org.linscreen.LinScreen.metainfo.xml"

grep -q '^Icon=org\.linscreen\.LinScreen$' "${desktop}"
grep -q '^Name=org\.linscreen\.LinScreen$' "${service}"
grep -q '<id>org\.linscreen\.LinScreen</id>' "${metainfo}"

if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "${desktop}"
fi
if command -v appstreamcli >/dev/null 2>&1; then
    appstreamcli validate --no-net "${metainfo}"
fi

echo "LinScreen staged installation is valid: ${install_root}"
