import pyecm
import os
import hashlib

print('--- Testing installed pyecm-tools package ---')
TEST_FILE = 'install_test.bin'
ECM_FILE = TEST_FILE + '.ecm'
DECODED_FILE = TEST_FILE + '.decoded'
FILE_SIZE_MB = 5
print(f"Creating temporary file '{TEST_FILE}' with size {FILE_SIZE_MB} MB...")
with open(TEST_FILE, 'wb') as f:
    f.write(os.urandom(FILE_SIZE_MB * 1024 * 1024))
with open(TEST_FILE, 'rb') as f:
    original_hash = hashlib.md5(f.read()).hexdigest()
print(f'Original file hash: {original_hash}')
try:
    print('\nTesting pyecm.encode...')
    pyecm.encode(TEST_FILE, ECM_FILE)
    print('✅ encode() completed successfully.')
except Exception as e:
    print(f'❌ ERROR in encode(): {e}')
    exit()
try:
    print('\nTesting pyecm.decode...')
    pyecm.decode(ECM_FILE, DECODED_FILE)
    print('✅ decode() completed successfully.')
except Exception as e:
    print(f'❌ ERROR in decode(): {e}')
    exit()
print('\nComparing hashes...')
with open(DECODED_FILE, 'rb') as f:
    decoded_hash = hashlib.md5(f.read()).hexdigest()
print(f'Decoded file hash: {decoded_hash}')
if original_hash == decoded_hash:
    print('\n🎉🎉🎉 SUCCESS! Files are identical. Package works correctly!')
else:
    print('\n❌❌❌ FAILURE! Hashes do not match. Something went wrong.')
print('\nCleaning up temporary files...')
os.remove(TEST_FILE)
os.remove(ECM_FILE)
os.remove(DECODED_FILE)
print('Done.')
