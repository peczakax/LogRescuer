import os
import filecmp
import sys
from datetime import datetime
import hashlib

def get_file_info(filepath):
    stat = os.stat(filepath)
    return {
        'size': stat.st_size,
        'mtime': datetime.fromtimestamp(stat.st_mtime),
        'hash': calculate_file_hash(filepath)
    }

def calculate_file_hash(filepath, block_size=65536):
    hasher = hashlib.sha256()
    with open(filepath, 'rb') as f:
        for block in iter(lambda: f.read(block_size), b''):
            hasher.update(block)
    return hasher.hexdigest()

def compare_files(file1, file2):
    info1 = get_file_info(file1)
    info2 = get_file_info(file2)
    
    differences = []
    if info1['size'] != info2['size']:
        differences.append(f"Size differs: {info1['size']} vs {info2['size']} bytes")
    if info1['mtime'] != info2['mtime']:
        differences.append(f"Modification time differs: {info1['mtime']} vs {info2['mtime']}")
    if info1['hash'] != info2['hash']:
        differences.append("Content differs (different hash)")
    return differences

def compare_folders(folder1, folder2, indent=""):
    has_differences = False
    dir_cmp = filecmp.dircmp(folder1, folder2)
    
    # Files only in first folder
    if dir_cmp.left_only:
        has_differences = True
        print(f"\n{indent}Only in {folder1}:")
        for item in dir_cmp.left_only:
            path = os.path.join(folder1, item)
            if os.path.isfile(path):
                info = get_file_info(path)
                print(f"{indent}  - {item} ({info['size']} bytes, modified: {info['mtime']})")
            else:
                print(f"{indent}  - {item}/ (directory)")

    # Files only in second folder
    if dir_cmp.right_only:
        has_differences = True
        print(f"\n{indent}Only in {folder2}:")
        for item in dir_cmp.right_only:
            path = os.path.join(folder2, item)
            if os.path.isfile(path):
                info = get_file_info(path)
                print(f"{indent}  - {item} ({info['size']} bytes, modified: {info['mtime']})")
            else:
                print(f"{indent}  - {item}/ (directory)")

    # Differing files
    if dir_cmp.diff_files:
        has_differences = True
        print(f"\n{indent}Files that differ between: \n\n {folder1} \n\n and \n\n {folder2}: \n\n")
        for item in dir_cmp.diff_files:
            file1 = os.path.join(folder1, item)
            file2 = os.path.join(folder2, item)
            differences = compare_files(file1, file2)
            print(f"{indent}  - {item}:")
            for diff in differences:
                print(f"{indent}    * {diff}")

    # Funny files (permission errors, etc.)
    if dir_cmp.funny_files:
        has_differences = True
        print(f"\n{indent}Files with access issues:")
        for item in dir_cmp.funny_files:
            print(f"{indent}  - {item}")

    # Recursively compare subdirectories
    for common_dir in dir_cmp.common_dirs:
        sub_folder1 = os.path.join(folder1, common_dir)
        sub_folder2 = os.path.join(folder2, common_dir)
        if compare_folders(sub_folder1, sub_folder2, indent + "  "):
            has_differences = True

    return has_differences

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 folder_tree_comparator.py <folder1> <folder2>")
        sys.exit(1)
    
    folder1 = os.path.abspath(sys.argv[1])
    folder2 = os.path.abspath(sys.argv[2])
    
    print(f"Comparing folders:")
    print(f"Folder 1: {folder1}")
    print(f"Folder 2: {folder2}")
    
    if not os.path.exists(folder1) or not os.path.exists(folder2):
        print("Error: One or both folders do not exist!")
        sys.exit(1)
        
    has_differences = compare_folders(folder1, folder2)
    
    if not has_differences:
        print("\nFolders are identical!")
    else:
        print("\nFolders have differences.")
        sys.exit(1)
