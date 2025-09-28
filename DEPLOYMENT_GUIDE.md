# StreamDAB Analyzer - Professional Deployment Guide

## 🚀 Commercial Deployment Package v1.1.0

**Professional ETI Stream Analyzer for Broadcast Industry**  
**ETSI EN 300 799 Compliant | Qt6 + C++20 Implementation**

---

## 📦 Available Deployment Packages

### Linux Distribution Packages

#### Debian/Ubuntu (.deb)
- **Package:** `StreamDAB-Analyzer-1.1.0-Linux.deb` (19.4 KB)
- **Architecture:** amd64
- **Dependencies:** Qt6 runtime libraries (>= 6.0.0)
- **Installation:** `sudo dpkg -i StreamDAB-Analyzer-1.1.0-Linux.deb`

#### Generic Linux (.tar.gz)
- **Package:** `StreamDAB-Analyzer-1.1.0-Linux.tar.gz` (18.1 KB)
- **Portable deployment for any Linux distribution**
- **Manual installation:** Extract and copy to `/usr/local/bin/`

### Windows Deployment (Planned)
- **MSI Installer:** Professional Windows Installer with Qt6 dependencies
- **NSIS Package:** Lightweight Windows installer option
- **Portable:** Standalone executable with embedded Qt6 libraries

### macOS Deployment (Planned)
- **DMG Package:** macOS disk image with application bundle
- **Application Bundle:** Native macOS .app package with code signing

---

## 🔧 System Requirements

### Minimum Requirements
- **OS:** Linux (Ubuntu 20.04+, Debian 11+, CentOS 8+, Fedora 35+)
- **Architecture:** x86_64 (amd64)
- **Memory:** 512 MB RAM
- **Storage:** 100 MB available space
- **Qt6 Runtime:** Version 6.0.0 or higher

### Recommended Requirements
- **OS:** Ubuntu 22.04 LTS or equivalent
- **Memory:** 2 GB RAM for real-time processing
- **Storage:** 1 GB for logs and analysis data
- **Network:** Gigabit Ethernet for ETI-over-IP streams
- **Display:** 1920x1080 minimum resolution

---

## 📋 Installation Instructions

### Ubuntu/Debian Installation

1. **Download Package**
   ```bash
   wget https://releases.streamdab.com/v1.1.0/StreamDAB-Analyzer-1.1.0-Linux.deb
   ```

2. **Install Dependencies**
   ```bash
   sudo apt update
   sudo apt install qt6-base-dev libqt6widgets6 libqt6gui6 libqt6network6
   ```

3. **Install Package**
   ```bash
   sudo dpkg -i StreamDAB-Analyzer-1.1.0-Linux.deb
   sudo apt-get install -f  # Fix any missing dependencies
   ```

4. **Launch Application**
   ```bash
   StreamDAB_Enhanced
   # Or from Applications menu: StreamDAB Analyzer
   ```

### Generic Linux Installation

1. **Extract Package**
   ```bash
   tar -xzf StreamDAB-Analyzer-1.1.0-Linux.tar.gz
   cd StreamDAB-Analyzer-1.1.0-Linux
   ```

2. **Install Manually**
   ```bash
   sudo cp usr/bin/StreamDAB_Enhanced /usr/local/bin/
   sudo cp usr/share/applications/streamdab-analyzer.desktop /usr/share/applications/
   sudo chmod +x /usr/local/bin/StreamDAB_Enhanced
   ```

3. **Update Desktop Database**
   ```bash
   sudo update-desktop-database
   ```

---

## 🏢 Enterprise Deployment

### Network Installation

For enterprise environments with multiple workstations:

1. **Create Network Repository**
   ```bash
   # On deployment server
   sudo mkdir -p /var/www/html/streamdab-packages
   sudo cp StreamDAB-Analyzer-1.1.0-Linux.deb /var/www/html/streamdab-packages/
   ```

2. **Client Installation Script**
   ```bash
   #!/bin/bash
   # enterprise-install.sh
   
   PACKAGE_SERVER="http://your-server.com/streamdab-packages"
   PACKAGE_NAME="StreamDAB-Analyzer-1.1.0-Linux.deb"
   
   wget "$PACKAGE_SERVER/$PACKAGE_NAME"
   sudo dpkg -i "$PACKAGE_NAME"
   sudo apt-get install -f
   ```

### Configuration Management

**Ansible Playbook Example:**
```yaml
---
- name: Deploy StreamDAB Analyzer
  hosts: broadcast_workstations
  become: yes
  tasks:
    - name: Install Qt6 dependencies
      apt:
        name:
          - qt6-base-dev
          - libqt6widgets6
          - libqt6gui6
          - libqt6network6
        state: present
        
    - name: Download StreamDAB package
      get_url:
        url: "{{ package_server }}/StreamDAB-Analyzer-1.1.0-Linux.deb"
        dest: "/tmp/StreamDAB-Analyzer-1.1.0-Linux.deb"
        
    - name: Install StreamDAB Analyzer
      apt:
        deb: "/tmp/StreamDAB-Analyzer-1.1.0-Linux.deb"
```

---

## 🚀 Application Features

### Core Capabilities
- **ETI File Processing:** Complete ETSI EN 300 799 compliant processing
- **Real-time Analysis:** ETI-over-IP stream monitoring with <17ms latency
- **Service Discovery:** Comprehensive DAB/DAB+ service enumeration
- **FIC Decoding:** Fast Information Channel processing with 11 FIG types
- **Professional UI:** Three-panel broadcast industry interface

### Professional Features
- **ETSI Compliance:** Full regulatory compliance validation
- **Performance Monitoring:** >900 FPS processing capability
- **Memory Optimization:** <100MB operational footprint
- **Multi-format Support:** ETI-NI, ETI-LI, EDI format compatibility
- **Thai DAB Standards:** Integrated Thai broadcasting standards support

---

## 🔧 Configuration

### Application Configuration

**Default Configuration File:** `~/.config/StreamDAB/settings.conf`

```ini
[General]
theme=professional
language=en_US
log_level=INFO

[ETI]
buffer_size=1024
processing_threads=4
real_time_mode=false

[Network]
multicast_address=239.192.0.1
port=9200
bind_interface=any

[Display]
update_rate=60
show_hex_view=true
enable_constellation=true
```

### System Integration

**Desktop Entry:** `/usr/share/applications/streamdab-analyzer.desktop`
```desktop
[Desktop Entry]
Version=1.0
Type=Application
Name=StreamDAB Analyzer
Comment=Professional ETI Stream Analyzer
Exec=/usr/local/bin/StreamDAB_Enhanced
Icon=streamdab-analyzer
Terminal=false
Categories=AudioVideo;Audio;Engineering;
```

---

## 🔍 Troubleshooting

### Common Issues

1. **Qt6 Library Missing**
   ```bash
   sudo apt install qt6-base-dev qt6-base-dev-tools
   ```

2. **Permission Denied**
   ```bash
   sudo chmod +x /usr/local/bin/StreamDAB_Enhanced
   ```

3. **Desktop Entry Not Visible**
   ```bash
   sudo update-desktop-database
   gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor
   ```

### Performance Optimization

**For High-Performance Systems:**
```bash
# Increase system limits
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf

# Optimize network buffers
sudo sysctl -w net.core.rmem_max=26214400
sudo sysctl -w net.core.rmem_default=26214400
```

---

## 📊 Deployment Validation

### Verification Script

```bash
#!/bin/bash
# validate-deployment.sh

echo "StreamDAB Analyzer Deployment Validation"
echo "========================================"

# Check installation
if command -v StreamDAB_Enhanced &> /dev/null; then
    echo "✓ StreamDAB_Enhanced executable found"
else
    echo "✗ StreamDAB_Enhanced executable not found"
    exit 1
fi

# Check Qt6 libraries
ldd $(which StreamDAB_Enhanced) | grep -q libqt6
if [ $? -eq 0 ]; then
    echo "✓ Qt6 libraries linked correctly"
else
    echo "✗ Qt6 libraries missing"
    exit 1
fi

# Check desktop integration
if [ -f "/usr/share/applications/streamdab-analyzer.desktop" ]; then
    echo "✓ Desktop integration installed"
else
    echo "⚠ Desktop integration not found"
fi

echo "Deployment validation completed successfully!"
```

---

## 📞 Support & Contact

**Commercial Support:**
- Email: support@streamdab.com
- Website: https://streamdab.com
- Documentation: https://docs.streamdab.com

**Technical Requirements:**
- ETSI EN 300 799 compliance questions
- Professional broadcast integration
- Enterprise deployment assistance
- Custom feature development

---

## 📄 License & Legal

**Commercial License Required for:**
- Commercial broadcast monitoring
- Professional broadcast environments
- Enterprise deployments
- Redistribution and resale

**Evaluation License Available for:**
- Technical evaluation (30 days)
- Academic research
- Non-commercial testing

Contact sales@streamdab.com for commercial licensing information.

---

*StreamDAB Analyzer v1.1.0 - Professional ETI Stream Analysis Solution*  
*Copyright © 2025 StreamDAB Technologies. All rights reserved.*