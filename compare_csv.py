import csv
import sys
import argparse
import math

def read_csv_to_dict(filepath):
    """
    Reads a CSV file into a dictionary keyed by Frame Number.
    Returns: (data_dict, fieldnames)
    data_dict structure: {frame_number: {col_name: value, ...}}
    """
    data = {}
    fieldnames = []
    
    try:
        with open(filepath, 'r', encoding='utf-8-sig') as f:
            reader = csv.DictReader(f)
            fieldnames = [fn.strip() for fn in reader.fieldnames]
            
            # Identify the Frame Number column
            frame_keys = ['Frame number', 'frame_index', 'frame']
            frame_key = next((k for k in fieldnames if k in frame_keys), None)
            
            if not frame_key:
                print(f"Error: Could not find frame number column (looked for {frame_keys}) in {filepath}")
                sys.exit(1)
                
            for row in reader:
                try:
                    # Strip whitespace from keys as well if DictReader didn't handle it
                    cleaned_row = {k.strip(): v.strip() for k, v in row.items()}
                    frame_num = int(cleaned_row[frame_key])
                    
                    processed_row = {}
                    for k, v in cleaned_row.items():
                        if k == frame_key:
                            continue
                        try:
                            # Strict conversion to float
                            processed_row[k] = float(v)
                        except (ValueError, TypeError):
                            # Fallback to 0.0 for non-numeric stuff but allow investigation
                            processed_row[k] = 0.0
                    
                    data[frame_num] = processed_row
                except ValueError:
                    continue # Skip invalid rows
                    
    except FileNotFoundError:
        print(f"Error: File not found - {filepath}")
        sys.exit(1)

    return data, fieldnames, frame_key

def compare_csvs(file1, file2, tolerance=1e-4):
    print(f"Comparing:\n A: {file1}\n B: {file2}\n")
    
    data1, fields1, key1 = read_csv_to_dict(file1)
    data2, fields2, key2 = read_csv_to_dict(file2)
    
    # Find common frames
    common_frames = sorted(list(set(data1.keys()) & set(data2.keys())))
    
    # Find common columns
    cols1 = set(data1[common_frames[0]].keys()) if common_frames else set()
    cols2 = set(data2[common_frames[0]].keys()) if common_frames else set()
    common_cols = sorted(list(cols1 & cols2))
    
    if not common_cols:
        print("Error: No common columns found to compare.")
        return

    # Output setup
    output_filename = "diff_result.csv"
    output_headers = ["Frame number"] + [f"{c}_diff" for c in common_cols]
    diff_count = 0
    
    try:
        with open(output_filename, 'w', newline='') as out_f:
            writer = csv.writer(out_f)
            writer.writerow(output_headers)
            
            for frame in common_frames:
                row1 = data1[frame]
                row2 = data2[frame]
                
                row_diffs = [frame]
                has_significant_diff = False
                
                for col in common_cols:
                    val1 = row1[col] # Already float from read_csv_to_dict
                    val2 = row2[col] # Already float
                    
                    diff_val = val1 - val2
                    
                    if abs(diff_val) > tolerance:
                        has_significant_diff = True
                        
                    row_diffs.append(diff_val)
                
                writer.writerow(row_diffs)
                if has_significant_diff:
                    diff_count += 1
                    
        print(f"Difference report saved to: {output_filename}")

    except IOError as e:
        print(f"Error writing output file: {e}")
    
    print("-" * 60)
    print(f"Comparison Complete.")
    print(f"Total Common Frames: {len(common_frames)}")
    print(f"Frames with Significant Differences (> {tolerance}): {diff_count}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compare two debug CSV files.")
    parser.add_argument("file1", help="Path to the first CSV file (e.g., linux_ version)")
    parser.add_argument("file2", help="Path to the second CSV file (reference)")
    
    args = parser.parse_args()
    
    compare_csvs(args.file1, args.file2)
