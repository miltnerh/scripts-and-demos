# This is the start of the nndc_parser script.

import pandas as pd
import mendeleev as mnv
import sqlite3
import numpy as np
import math
import regex as re

# Dictionary mapping for handling abbreviations or non-ASCII characters from the NNDC site.
# Unrecognized entries to normalized entries for isomer database.
# Also contains time mappings for the microsecond conversions in T12

__all__ = ["sqlite_characterize_table","sqlite_to_pandas", "parse_isomerID",
            "stretch_CSV", "sqlite_entries", "input_entries"]

entry_map = {
        '<': 'LT', '>': 'GT', '<=': 'LE', '>=': 'GE', 'approx': 'AP',
        
        'm': 60, 's': 1, 'ms': 1e-3, 'us': 1e-6, 'ns': 1e-9, 'ps': 1e-12, 'fs': 1e-15,
        
        }

def sqlite_characterize_table(database_dir, subtable):
    """Open connection to the desired .sqlite database and extracts existent columns and the most recent GUID (entry number).
    
    
    Args:
        database_dir (str): Directory for desired database.
        subtable (str): Name of subtable to convert to DataFrame object.

    Returns:
        database_cols (list): List of column names in .sqlite file table.
        recent_GUID (int): Latest ROW entry (for GUID tracking).
    """
    
    conn = sqlite3.connect(database_dir)
    database_cols = pd.read_sql_query(f'SELECT name FROM pragma_table_info("{subtable}") ORDER BY cid', conn)
    database_cols = list(database_cols['name'])
    recent_GUID = pd.read_sql_query(f'SELECT MAX(ROW) FROM {subtable}', conn)
    recent_GUID = int(recent_GUID.iloc[0,0])
    conn.close()

    return database_cols, recent_GUID


def sqlite_to_pandas(database_dir, subtable):
    """Choose some table from a sqlite database to convert to a Pandas DataFrame.

    Args:
        database_dir (str): Directory for desired database.
        subtable (str): Name of subtable to convert to DataFrame object.

    Returns:
        pandas.DataFrame: sqlite subtable as Pandas DataFrame.
    """
     
    
    conn = sqlite3.connect(database_dir)
    sql_DF = pd.read_sql_query('select * from ' + subtable, conn, index_col=None)
    conn.close()
    
    return sql_DF


def parse_isomerID(filename):
    """Takes Isomer filename of shape 'adoptedLevels###SYMBOL.csv' shape and extracts the atomic mass and proton number to build INDEX_IT identifiers.
        This program is idealized for NNDC CSV file downloads (hence the required shape).

    Args:
        filename (str): File name of style 'adoptedLevels###SYMBOL.csv'.

    Returns:
        el_mass (str): A number (atomic mass) returned as string.
        el_symbol (str): Chemical symbol of element from 'SYMBOL' section of filename.
        el_atnum (str): Z number (atomic number) of element.
    
    Raises:
        ValueError: If file name is not compatible with the requested format.
    """
    
    int_list = []
    str_list = []
    interest = filename[13:-4]
    try:
        for el in list(interest):
            try:
                int(el)
                int_list.append(el)
            except:
                str_list.append(el)
                
        el_symbol = ''.join(str_list)
        el_mass = ''.join(int_list)
        el_atnum = str(mnv.element(el_symbol).atomic_number)
   
        return el_mass, el_symbol, el_atnum
    except:
        raise ValueError("Check filename format (force 'adoptedLevels###SYMBOL.csv' shape)")


def stretch_CSV(filename):
    """ Performs a series of parses and expansions on a designated CSV (from the NNDC)
        to a form where the rows are gamma energy readings.

    Args:
        filename (str): NNDC CSV file name directory.

    Returns:
        pandas.DataFrame: Dataframe with rows of gamma energy readings.
    """
    # Read in CSV of filename
    isom_csv = pd.read_csv(filename, index_col = False)
    # NNDC formatting contains extra information, perform cut on CSV
    if any(isom_csv['E(level)(keV)'] == 'E(level)(keV)'):
        cut_index = isom_csv[isom_csv['E(level)(keV)']== 'E(level)(keV)'].index[0]
        isom_csv = isom_csv[:cut_index]

    # Rename last column, formatted as 'Unnamed: #' by pd.read_csv
    isom_csv.rename(columns={isom_csv.columns[-1]: 'Final Jπ'}, inplace= True)

    # This is from AI, uses regex to remove non ASCII charracters and clean the data.
    # Effectively normalizes Pandas dataframe of CSV.
    # Function to clean strings (removes invisible characters, e.g., \u200b, \xa0, etc.)
    def clean_string(s):
        if isinstance(s, str):
            # Handles unexpected greater than or less than entries
            s = s.replace('≤', '<=')
            s = s.replace('≥', '>=')
            s = s.replace('?', '')
            s = s.replace('β', 'B')
            s = s.replace('ε', 'lc_sigma')
            s = s.replace('≈', 'approx ')
            s = s.replace('S', '')
            return re.sub(r"[^\x20-\x7E]", "", s).strip()  # keep only printable ASCII
        return s
    
    # Apply cleaning to the entire DataFrame
    isom_csv = isom_csv.map(clean_string)
    isom_csv = isom_csv.dropna(subset = ["E(γ)(keV)"]).reset_index(drop = True)

    # Back-filling decay cascade of T1/2 from parent
    isom_csv['RATIOS'] = ''
    isom_csv['DECAY_PARENT'] = ''
    nan_count = 0
    for i in range(isom_csv.shape[0]):
        try:
            if np.isnan(isom_csv.loc[i, 'T1/2(level)']):
                nan_count += 1
            
        except:
            isom_csv.loc[i-nan_count:i, 'T1/2(level)'] = isom_csv.loc[i, 'T1/2(level)']
            if i-nan_count != i:
                isom_csv.loc[i-nan_count:i, 'DECAY_PARENT'] = isom_csv.loc[i, 'E(level)(keV)']
            nan_count = 0
    isom_csv = isom_csv.dropna(subset = 'T1/2(level)')

    # Enforce lists on viable rows
    exp_cols = ["E(γ)(keV)","I(γ)","M(γ)","Final Levels", "Final Jπ"]
    missing_cols = [] # For 're-filling' missing columns in appropriate shape for exploding

    # small prep for parsing M(γ) and Final Jπ column
    # States have multiplicities and shared formatting inconsistencies, handle together
    for multi_state in ["M(γ)","Final Jπ"]:
        if multi_state in list(isom_csv.columns):
            isom_csv[multi_state] = isom_csv[multi_state] + ' '

    # Splitting for exploding rows per E(γ)
    for col in exp_cols:
        try:
            if col in ["E(γ)(keV)","I(γ)","Final Levels"]:
                isom_csv[col] = isom_csv[col].str.split(",")
            elif col in ["Final Jπ","M(γ)"]:
                isom_csv[col] = isom_csv[col].str.split(", ")
        except:
            print(f'Column {col} not found')
            missing_cols.append(col)

    # Create missing columns that we are expecting for the normalized data
    for col in missing_cols:
        isom_csv[col] = isom_csv['E(γ)(keV)'].apply(lambda lst: [np.nan for _ in lst])

    # Explode the columns with lists of entries (per gamma energy) into individual rows per gamma energy
    # Uses pandas.explode()
    exp_cols = ["E(γ)(keV)","I(γ)","M(γ)","Final Levels", "Final Jπ"]
    isom_csv = isom_csv.explode(exp_cols).reset_index(drop=True)

    # now set space separation for units (T1/2) and uncertainties
    unc_cols = ['E(level)(keV)', 'E(γ)(keV)','I(γ)']
    for col in unc_cols:
        isom_csv[col] = isom_csv[col].str.strip() # clear leading space
        isom_csv[col] = isom_csv[col].str.split(" ")

    # Reorder CSV columns into ideal shape for reading
    # REDEFINE THE REASSIGNED ORDER TO INCLUDE ANY ADDED COLUMNS
    isom_csv = isom_csv[['E(level)(keV)', 'XREF', 'Jπ(level)', 'T1/2(level)', 'E(γ)(keV)', 'I(γ)', 'M(γ)', 'Final Levels', 'Final Jπ', 'RATIOS', 'DECAY_PARENT']]

    # Clean entries to be accessible for drawing database inputs
    # Also, hand T1/2 entries as they are more complicated with ratios
    isom_csv['T1/2(level)'] = isom_csv['T1/2(level)'].str.split(',')

    for i in range(isom_csv.shape[0]):
        # First work on the columns with only uncertainties ('E(level)(keV)', 'E(γ)(keV)','I(γ)')
        for col in unc_cols:
            unc_entry = isom_csv.loc[i,col]
            # Check if the entry us a list and its length
            if type(unc_entry) == list:
                if len(unc_entry) != 2:
                    # Add space to ending to normalize, creates null uncertainty
                    isom_csv.at[i,col] = unc_entry + ['']

        # Trim extra entries for half-life information
        t12_entry = isom_csv.loc[i,'T1/2(level)']
        if isinstance(t12_entry, list):
            if len(t12_entry) > 1:
                isom_csv.at[i,'T1/2(level)'] = t12_entry[0].split(' ')
                isom_csv.at[i,'RATIOS'] += str(t12_entry[1:])
            else:
                isom_csv.at[i,'T1/2(level)'] = t12_entry[0].split(' ')
            #reinitialize entry
            t12_entry = isom_csv.loc[i,'T1/2(level)']
            if t12_entry[0] in entry_map:
                isom_csv.at[i,'T1/2(level)'] = [t12_entry[1], t12_entry[2], t12_entry[0]]
        else:
            isom_csv.at[i,'DECAY_PARENT'] += 'UNKWN'

        # Another handling for uncertainty order of I(γ) similar to above
        Igam_entry = isom_csv.loc[i,'I(γ)']
        if isinstance(Igam_entry, list):
            if Igam_entry[0] in entry_map:
                isom_csv.at[i,'I(γ)'] = [Igam_entry[1], entry_map[Igam_entry[0]]]
                
    return isom_csv


def sqlite_entries( filename, user_name, source = 'NNDC', ref_DB = None, db_dir = None, subtable = None, input_entries = False, extra_tables = False ):
    """Performs complete drawing of entries with creation of expanded table 

    Args:
        filename (str): File name of style 'adoptedLevels###SYMBOL.csv' to draw entries for the database.
        user_name (str): Initials of user who are inputting the entries
        source (str, optional): Contains the source information. Defaults to 'NNDC' as that is what the program is written for.
        ref_DB (str, optional): Initialized reference database from the .sqlite file (recommended for importing many files, operate one existent dataframe).
        db_dir (str, optional): Directory to DataBase. HIGHLY recommend running script in same directory as BOTH database and CSV.
        subtable (str, optional): Table within the database to access.
        extra_tables (bool, optional): Will also return the expanded CSV (exp_CSV), isomer database (Isom_DB) values if True. Defaults to False.
        input_entries (bool)

    Returns:
        pandas.DataFrame: Entries formatted for Isom_DB formatting as of 6/4/2025 for LISE++ program to access.
        pandas.DataFrame, pandas.DataFrame, pandas.DataFrame (optional): Includes all dataframes from sqlite drawing process
    
    Raises:
        ValueError: Need some reference database to perform the operation.
    """
    
    # Perform database stretch
    exp_CSV = stretch_CSV(filename)

    if all(s is None for s in [ref_DB, db_dir, subtable]):
            raise ValueError('Must have reference to sqlite DataBase (either by directory or instanced variable)')          
    elif ref_DB is None:
        template_cols, max_GUID = sqlite_characterize_table(db_dir, subtable)
        ROW_GUID = max_GUID + 1
        conn = sqlite3.connect(db_dir)
    else:
        Isom_DB = ref_DB
        template_cols = list(Isom_DB.columns)
        ROW_GUID = max(Isom_DB['ROW']) + 1
        
    # Initialize some order rules (next row GUID, new table to be appended, INDEX_IT first digits)

    N_inputs = exp_CSV.shape[0]
    new_entries = pd.DataFrame(np.full( (N_inputs, len(template_cols)), fill_value = np.nan, dtype=object ), columns = template_cols)

    A, Sym, Z = parse_isomerID(filename)

    # Row iteration for drawing new inputs 
    for i in range(N_inputs):
        focus = exp_CSV.loc[i, :]
        E_GAM, DE_GAM = focus['E(γ)(keV)']
        INDEX_IT = round(float(A + Z + E_GAM))

        # handle NaN entries
        if isinstance(focus['T1/2(level)'], list):
            T12, scale, DT12 = focus['T1/2(level)']
            T12_factor = entry_map[scale]
            if DT12 in entry_map:
                DT12 = entry_map[DT12]
            elif DT12[0] == '+':
                min_index = DT12.index('-')
                try:
                    plus = int(DT12[1:min_index])
                    minus = int(DT12[min_index+1:])
                    DT12 = (plus + minus)/2 * T12_factor / 1e-6
                except:
                    raise ValueError('Non-integer in DT12 range [sqlite_entries]')
            else:
                try:
                    DT12 = T12_factor * float(DT12) * 10**len(str(DT12))
                except:
                    # unexpected value handling
                    DT12 = DT12 + ' ' + str(T12_factor / 1e-6)
                    raise ValueError('Unexpected T12 Entry [sqlite_entries]')
            T12 = T12_factor / 1e-6 * float(T12)
        else:
            T12, DT12 = (np.nan, np.nan)
        
        LEVEL = focus['E(level)(keV)'][0]
        D_LEVEL = focus['E(level)(keV)'][1]

        # assign remaining entries
        # E_FIN = focus['Final Levels']
        JPI = focus['Final Jπ']
        # try/except for NaN inputs (cannot unpack np.nan problem)
        try:
            I_GAM, DI_GAM = focus['I(γ)']
        except:
            I_GAM, DI_GAM = (np.nan, np.nan)

        M_GAM = focus['M(γ)']
        ROW = int(ROW_GUID + i) 

        # Normalize for database
        new_entries[new_entries == ''] = np.nan

        # ordering for Isomer_DB.sqlite
        new_entries.loc[i,['INDEX_IT', 'A_IT', 'Z_IT', 'E_GAMMA', 'D_EG', 'IT_RATIO',
            'T12', 'D_T12', 'LEVEL', 'D_LEVEL', 'JPI', 'I_GAMMA', 'D_IG', 'M_GAMMA',
            'SOURCE', 'NAME', 'ROW']] = [str(INDEX_IT), A, Z, E_GAM, DE_GAM, 10, T12, DT12, LEVEL, D_LEVEL, 
                                        JPI, I_GAM, DI_GAM, M_GAM, source, user_name, ROW ]
            
            
    if conn and input_entries:
        try:
            new_entries.to_sql(subtable, conn, if_exists='append', index=False)
            conn.close()
        except:
            raise ValueError("No connection established or directory given")
    conn.close()
    if extra_tables: return new_entries, exp_CSV
    else: return new_entries


def input_entries(entries, db_dir, subtable):
    """ Takes created entries (from 'sqlite_entries') and appends them to a subtable of the sqlite database.

    Args:
        entries (pandas.DataFrame): New entries to append to the table.
        db_dir (str): .sqlite database directory
        subtable (str): Subtable within database to append to.
    """

    conn = sqlite3.connect(db_dir)
    entries.to_sql(subtable, conn, if_exists='append', index=False)
    conn.close()