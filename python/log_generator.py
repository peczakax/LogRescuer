import random
from datetime import datetime, timedelta
import codecs
import os
import shutil

# Add time periods
TIME_PERIODS = {
    'ancient': {
        'start_year': 1970,
        'end_year': 1979,
        'components': ['MainFrame', 'PunchCard', 'TapeReader', 'BatchProcessor'],
        'operations': ['LOAD', 'PUNCH', 'PRINT', 'COMPUTE'],
        'messages': [
            'Batch job {} on unit {}',
            'Tape mount {} requested',
            'Card deck {} processed',
            'Memory bank {} status: {}'
        ]
    },
    'modern': {
        'start_year': 2020,
        'end_year': 2023,
        'components': ['Database', 'UserService', 'Authentication', 'FileSystem', 'NetworkManager'],
        'operations': ['READ', 'WRITE', 'DELETE', 'UPDATE', 'SYNC'],
        'messages': [
            'Connection attempt {}',
            'Transaction {} completed',
            'Request {} processed',
            'Session {} expired'
        ]
    },
    'future': {
        'start_year': 2100,
        'end_year': 2150,
        'components': ['QuantumCore', 'HoloInterface', 'NeuralNet', 'BionicSystem', 'TimeSync'],
        'operations': ['MATERIALIZE', 'FOLD', 'SYNTHESIZE', 'QUANTUM_SYNC', 'NEURAL_PROCESS'],
        'messages': [
            'Quantum entanglement {} achieved',
            'Neural pathway {} established',
            'Holographic interface {} initialized',
            'Time variance {} detected in sector {}'
        ]
    }
}

def generate_log_entry():
    log_levels = ['INFO', 'WARNING', 'ERROR', 'DEBUG', 'CRITICAL']
    status = ['SUCCESS', 'FAILED', 'IN_PROGRESS', 'TIMEOUT', 'RETRY']
    
    # Select random time period
    period_name, period = random.choice(list(TIME_PERIODS.items()))
    
    # Generate timestamp within the selected period
    start_date = datetime(period['start_year'], 1, 1)
    end_date = datetime(period['end_year'], 12, 31)
    time_delta = end_date - start_date
    random_days = random.randrange(time_delta.days)
    timestamp = start_date + timedelta(
        days=random_days,
        hours=random.randint(0, 23),
        minutes=random.randint(0, 59),
        seconds=random.randint(0, 59)
    )
    
    message = random.choice(period['messages']).format(
        random.choice(status),
        f"unit_{random.randint(1000, 9999)}" if "{}" in random.choice(period['messages']) else ""
    )
    
    return f"[{timestamp.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]}] {random.choice(log_levels)} [{random.choice(period['components'])}] [{random.choice(period['operations'])}] - {message}"

def create_log_file(filepath, min_entries, max_entries):
    num_entries = random.randint(min_entries, max_entries)
    with codecs.open(filepath, 'w', encoding='utf-16') as f:
        for _ in range(num_entries):
            f.write(generate_log_entry() + '\n')
    return filepath

def create_directory_structure(base_path):
    # Define the directory structure with time machine components
    structure = {
        'temporal_core': ['flux_capacitor', 'chronosphere', 'time_dilation_unit'],
        'quantum_matrix': ['entanglement_node', 'timeline_stabilizer'],
        'temporal_lab': ['paradox_chamber', 'timeline_observer']
    }
    
    # Create directories and return their paths
    paths = []
    for sector, devices in structure.items():
        for device in devices:
            path = os.path.join(base_path, sector, device, 'temporal_logs')
            os.makedirs(path, exist_ok=True)
            paths.append(path)
    return paths

def main():
    base_dir = 'time_machine_logs'
    if os.path.exists(base_dir):
        shutil.rmtree(base_dir)
    
    # Create directory structure
    log_dirs = create_directory_structure(base_dir)
    
    # Define file templates with size ranges
    file_templates = [
        ('temporal_core.log', 500, 1000),
        ('quantum_fluctuations.log', 200, 500),
        ('paradox_warnings.log', 50, 200),
        ('timeline_access.log', 300, 600),
        ('entropy_debug.log', 100, 300)
    ]
    
    # Track created files for duplication
    original_files = []
    
    # Create original files in first directory
    primary_dir = log_dirs[0]
    for filename, min_entries, max_entries in file_templates:
        filepath = os.path.join(primary_dir, filename)
        create_log_file(filepath, min_entries, max_entries)
        original_files.append((filename, filepath))
        print(f"Created original: {filepath}")
    
    # Process other directories
    for dir_path in log_dirs[1:]:
        # Duplicate some files (30% chance for each file)
        for filename, source_path in original_files:
            if random.random() < 0.3:
                target_path = os.path.join(dir_path, filename)
                shutil.copy2(source_path, target_path)
                print(f"Duplicated: {target_path}")
        
        # Create some unique files
        unique_name = f"temporal_anomaly_{os.path.basename(dir_path)}_{random.randint(1000,9999)}.log"
        unique_path = os.path.join(dir_path, unique_name)
        create_log_file(unique_path, 100, 400)
        print(f"Created unique: {unique_path}")

if __name__ == "__main__":
    main()
