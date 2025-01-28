const fs = require('fs');
const os = require('os');
const path = require('path');

const dns = [
    '1.1.1.1',
    '8.8.8.8',
    '3.3.3.3'
];

async function DNSFileCheck() {
    const homedir = os.homedir();
    let targetPath, filedir = 'dns.json';

    if (os.platform() === "linux") {
        targetPath = path.join(homedir, '.config', 'hey403');
    } else if (os.platform() === "win32") {
        targetPath = path.join(homedir, 'Documents', 'hey403');
    } else {
        throw new Error('Unsupported operating system');
    }

    if (!fs.existsSync(targetPath)) {
        fs.mkdirSync(targetPath, { recursive: true });
    }

    const filePath = path.join(targetPath, filedir);

    if (!fs.existsSync(filePath)) {

        fs.writeFileSync(filePath, JSON.stringify(dns, null, 2), 'utf8');
        console.log('File created with DNS list.');
        return dns;
    } else {

        const data = fs.readFileSync(filePath, 'utf8');
        try {
            
            const dnsList = JSON.parse(data);
            console.log('DNS list read from file.');
            return dnsList;

        } catch (err) {
            console.error('Error parsing DNS file:', err);
            throw new Error('Invalid DNS file format');
        }
    }
}

module.exports = DNSFileCheck;