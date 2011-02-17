# This script takes a file that was created using infix_data.* scripts,
# and recreates the original file.
BEGIN { for (i = 1; i <= 26; i++) { k = i + 65; ct[sprintf("%c", k)] = i; }}
{
  n = split($0, s, /\+/);
  infix_pos = ct[substr(s[2], 1, 1)];
  infix_len = ct[substr(s[2], 2, 1)];
  suffix_len = ct[substr(s[2], 3, 1)];
#  printf"%s\n", $0;
#  printf "infix_pos=%d, infix_len=%d, suffix_len=%d\n", infix_pos,
#    infix_len, suffix_len;
  l = length(s[1]);
  printf "%s\t", s[1];
  if (infix_pos > 0) {
    # There is an infix
    if (infix_len > 0) {
      printf "%s", substr(s[1], 1, infix_pos);
    }
    if (l - infix_pos - infix_len - suffix_len > 0) {
      printf "%s", substr(s[1], infix_pos + infix_len + 1,
			  l - infix_pos - infix_len - suffix_len);
    }
  }
  else {
    # There is a prefix
    if (infix_len > 0) {
      # Yes, there is a prefix
      printf "%s", substr(s[1], 1+ infix_len, l - suffix_len - infix_len);
    }
    else {
      # Well, in fact there is neither a prefix, nor an infix
      printf "%s", substr(s[1], 1, l - suffix_len);
    }
  }
  if (length(s[2]) > 3) printf "%s", substr(s[2], 4);
  printf "\t%s\n", s[3];
}

  
