#!/bin/bash
# ETI Stream Analyzer - RPM Post-Installation Script
# Professional desktop integration for Red Hat/Fedora/SUSE systems

# Professional application configuration
APP_NAME="ETI Stream Analyzer"
DESKTOP_FILE="/usr/share/applications/eti-stream-analyzer.desktop"
ICON_PATH="/usr/share/pixmaps/eti-stream-analyzer.png"
MIME_TYPE_FILE="/usr/share/mime/packages/eti-stream-analyzer.xml"

echo "Configuring ETI Stream Analyzer for professional broadcast use..."

# Professional directory structure
mkdir -p /var/log/eti-stream-analyzer
mkdir -p /etc/eti-stream-analyzer
mkdir -p /usr/share/eti-stream-analyzer/samples
mkdir -p /usr/share/doc/eti-stream-analyzer

# Set professional permissions
chmod 755 /var/log/eti-stream-analyzer
chmod 755 /etc/eti-stream-analyzer
chown root:root /var/log/eti-stream-analyzer
chown root:root /etc/eti-stream-analyzer

# Professional system integration
/sbin/ldconfig

# Update desktop database for professional integration
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications &>/dev/null || true
fi

# Update MIME database for broadcast file support
if command -v update-mime-database >/dev/null 2>&1; then
    update-mime-database /usr/share/mime &>/dev/null || true
fi

# Update icon cache for professional appearance
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor &>/dev/null || true
fi

# Professional SELinux context (if SELinux is enabled)
if command -v selinuxenabled >/dev/null 2>&1 && selinuxenabled; then
    if command -v restorecon >/dev/null 2>&1; then
        restorecon -R /usr/bin/ETIStreamAnalyzer &>/dev/null || true
        restorecon -R /usr/share/eti-stream-analyzer &>/dev/null || true
    fi
fi

echo "ETI Stream Analyzer professional configuration complete."
echo "Application ready for broadcast engineering workflows."

exit 0
