#!/bin/bash
# Post-installation script for StreamDAB Analyser
# RPM-based distributions (Fedora/RHEL/SUSE) post-installation

# Update MIME database for ETI file associations
if [ -x /usr/bin/update-mime-database ]; then
    /usr/bin/update-mime-database /usr/share/mime >/dev/null 2>&1 || true
fi

# Update desktop database for application menu
if [ -x /usr/bin/update-desktop-database ]; then
    /usr/bin/update-desktop-database >/dev/null 2>&1 || true
fi

# Update icon cache
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    /usr/bin/gtk-update-icon-cache -f -t /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi

# Reload systemd if present (for potential future service integration)
if [ -x /usr/bin/systemctl ]; then
    /usr/bin/systemctl daemon-reload >/dev/null 2>&1 || true
fi

# SELinux context restoration for RHEL/CentOS/Fedora
if [ -x /usr/sbin/restorecon ]; then
    /usr/sbin/restorecon -R /usr/bin/StreamDABAnalyser >/dev/null 2>&1 || true
    /usr/sbin/restorecon -R /usr/share/applications/eti-stream-analyser.desktop >/dev/null 2>&1 || true
fi

# Create broadcast group for professional users
if ! getent group broadcast >/dev/null 2>&1; then
    groupadd -r broadcast 2>/dev/null || true
fi

echo "StreamDAB Analyser installation completed successfully"
echo "Professional ETI analysis tools are now available"

exit 0
