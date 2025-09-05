#!/bin/bash

WEBSERV="./webserv"
TMPFILE="test.conf"
DUMMY_WRONG="dummy.txt"

# Define tests as "name|config|expected_error_substring"
tests=(
  # --- File errors ---
  "wrong_extension|server { listen 8080; }|Invalid file extension"
  "file_not_found|missing.conf|Couldn't open config file"

  # --- Server block errors ---
  "missing_brace|server (|Expected '{' after 'server'"
  "invalid_directive|server {
    listen 8080;
    foo bar;
  }|directive is not allowed here"
  "missing_listen|server {
    host 127.0.0.1;
  }|Missing port value"

  # --- Host errors ---
  "empty_host|server {
    listen 8080;
    host;
  }|Missing host value"

  "missing_octet|server {
    listen 8080;
    host 192.168.1;
  }|Invalid host IP address"

  "empty_octet|server {
    listen 8080;
    host 192..1.1;
  }|Host address incomplete"

  "leading_zero|server {
    listen 8080;
    host 192.168.01.1;
  }|Host address contains leading zeros"

  "nonnumeric_octet|server {
    listen 8080;
    host 192.abc.0.1;
  }|Host address must include digits only"

  "out_of_range_octet|server {
    listen 8080;
    host 999.0.0.1;
  }|Invalid host IP address"

  # --- Listen errors ---
  "missing_port|server {
    listen;
  }|Missing port value"

  "extra_tokens_listen|server {
    listen 8080 extra;
  }|Port value contains unexpected spaces"

  "nonnumeric_port|server {
    listen abc;
  }|Port must include digits only"

  "invalid_port_zero|server {
    listen 0abc;
  }|Port must include digits only"

  "out_of_range_port|server {
    listen 70000;
  }|out of range"

  # --- Error page errors ---
  "missing_error_page_args|server {
    listen 8080;
    error_page 404;
  }|missing value"

  "extra_error_page_args|server {
    listen 8080;
    error_page 404 page.html extra;
  }|unexpected spaces"

  "nonnumeric_error_code|server {
    listen 8080;
    error_page abc page.html;
  }|Error code must include digits only"

  "out_of_range_error_code|server {
    listen 8080;
    error_page 200 page.html;
  }|not a valid HTTP error code"

  # --- Client max body size errors ---
  "missing_body_size|server {
    listen 8080;
    client_max_body_size;
  }|Missing argument"

  "unknown_unit|server {
    listen 8080;
    client_max_body_size 10X;
  }|Unknown size unit"

  "invalid_body_size_number|server {
    listen 8080;
    client_max_body_size 0;
  }|Invalid numeric value"

  # --- Location errors ---
  "location_no_path|server {
    listen 8080;
    location;
  }|Missing path value"

  "location_no_slash|server {
    listen 8080;
    location path;
  }|Path missing starting slash"

#   "location_extra_tokens|server {
#     listen 8080;
#     location /path extra;
#   }|unexpected spaces"

  "location_illegal_chars|server {
    listen 8080;
    location /p@th;
  }|illegal characters"

  "invalid_directive_in_location|server {
    listen 8080;
    location / {
      foo bar;
    }
  }|directive is not allowed here"

  # --- Root errors ---
  "missing_root|server {
  listen 8080;
  location / {
    root;
  }
}|Missing root value"

"extra_root_tokens|server {
  listen 8080;
  location / {
    root /var/www extra;
  }
}|unexpected spaces"

"root_no_slash|server {
  listen 8080;
  location / {
    root var/www;
  }
}|must start with '/'"

"root_illegal_chars|server {
  listen 8080;
  location / {
    root /va@r/www;
  }
}|illegal characters"

  # --- Index errors ---
    "missing_index|server {
    listen 8080;
    location / {
        index;
    }
    }|Missing index value"

    "invalid_index_chars|server {
    listen 8080;
    location / {
        index pa@ge.html;
    }
    }|illegal characters"


  # --- Allowed methods errors ---
    "missing_methods|server {
    listen 8080;
    location / {
        allowed_methods;
    }
    }|Missing allowed_methods value"

    "unsupported_method|server {
    listen 8080;
    location / {
        allowed_methods PUT;
    }
    }|unsupported HTTP method"

  # --- Return errors ---
  "return_missing_args|server {
    listen 8080;
    location / {
      return 301;
    }
  }|missing value"

  "return_extra_args|server {
    listen 8080;
    location / {
      return 301 /path extra;
    }
  }|unexpected spaces"

  "return_nonnumeric_code|server {
    listen 8080;
    location / {
      return abc /path;
    }
  }|Redirection code must include digits"

  "return_invalid_code|server {
    listen 8080;
    location / {
      return 200 /path;
    }
  }|not a valid HTTP redirection code"

  "return_empty_url|server {
    listen 8080;
    location / {
      return 301 \"\";
    }
  }|Redirection target must not be empty"

  # --- Upload path errors ---
  "missing_upload_path|server {
    listen 8080;
    location / {
      upload_path;
    }
  }|Missing upload_path value"

  "upload_path_root|server {
    listen 8080;
    location / {
      upload_path /;
    }
  }|Upload path cannot be root"

  # --- Autoindex errors ---
  "missing_autoindex|server {
    listen 8080;
    location / {
      autoindex;
    }
  }|Missing autoindex value"

  "extra_autoindex_tokens|server {
    listen 8080;
    location / {
      autoindex on extra;
    }
  }|unexpected extra tokens"

  "invalid_autoindex|server {
    listen 8080;
    location / {
      autoindex yes;
    }
  }|Incorrect syntax for autoindex"

  # --- CGI extension errors ---
  "missing_cgi_ext|server {
    listen 8080;
    location / {
      cgi_extension;
    }
  }|Exactly one CGI extension"

  "empty_cgi_ext|server {
    listen 8080;
    location / {
      cgi_extension \"\";
    }
  }|CGI extension must start with a '.' character."

  "no_dot_cgi_ext|server {
    listen 8080;
    location / {
      cgi_extension php;
    }
  }|must start with a '.'"

  "invalid_cgi_ext|server {
    listen 8080;
    location / {
      cgi_extension .ph@p;
    }
  }|Invalid CGI extension"
)

# --- Runner ---
for t in "${tests[@]}"; do
  name="${t%%|*}"
  rest="${t#*|}"
  conf="${rest%%|*}"
  expected="${rest#*|}"

  echo "---- Running test: $name ----"

  # Handle special file-level cases
  if [[ "$name" == "wrong_extension" ]]; then
    echo "$conf" > "$DUMMY_WRONG"
    conf="$DUMMY_WRONG"
  fi

  if [[ "$name" == "file_not_found" ]]; then
    # Just leave conf as a nonexistent file
    :
  else
    # Write normal server configs to TMPFILE
    if [[ "$name" != "wrong_extension" ]]; then
      echo "$conf" > "$TMPFILE"
      conf="$TMPFILE"
    fi
  fi

  # Run webserv
  output=$($WEBSERV "$conf" 2>&1)
  status=$?

  # Check results
  if [ $status -eq 0 ]; then
    echo "❌ $name FAILED (webserv did not exit with error)"
  else
    if [[ "$output" == *"$expected"* ]]; then
      echo "✅ $name passed (matched: $expected)"
    else
      echo "❌ $name FAILED"
      echo "Expected: $expected"
      echo "Got: $output"
    fi
  fi
  echo
done

# Cleanup
rm -f "$TMPFILE" "$DUMMY_WRONG"
