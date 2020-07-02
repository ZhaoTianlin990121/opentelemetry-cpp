#include <vector>
#include "opentelemetry/nostd/string_view.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace trace {
public abstract class TraceState {
  private:
    static const int KEY_MAX_SIZE = 256;
    static const int VALUE_MAX_SIZE = 256;
    static const int MAX_KEY_VALUE_PAIRS = 32;
    static const TraceState DEFAULT = TraceState.builder().build();

    static TraceState create(std::vector<Entry> entries) {
      // I don't know what exactly this is and how to replace this
      return new AutoValue_TraceState(Collections.unmodifiableList(entries));
    }

    // Value is opaque string up to 256 characters printable ASCII RFC0020 characters (i.e., the range
    // 0x20 to 0x7E) except comma , and =.
    static bool validateValue(nostd::string_view value) {
      if (value.length() > VALUE_MAX_SIZE || value[value.length() - 1] == ' ' /* '\u0020' */) {
        return false;
      }
      for (int i = 0; i < value.length(); i++) {
        char c = value[i];
        if (c == ',' || c == '=' || c < ' ' /* '\u0020' */ || c > '~' /* '\u007E' */) {
          return false;
        }
      }
      return true;
    }

    static bool isNumberOrDigit(char ch) {
      return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
    }

    // Key is opaque string up to 256 characters printable. It MUST begin with a lowercase letter, and
    // can only contain lowercase letters a-z, digits 0-9, underscores _, dashes -, asterisks *, and
    // forward slashes /.  For multi-tenant vendor scenarios, an at sign (@) can be used to prefix the
    // vendor name.
    // todo: benchmark this implementation
    static bool validateKey(nostd::string_view key) {
      if (key.length() > KEY_MAX_SIZE || key.isEmpty() || !isNumberOrDigit(key[0]))) {
        return false;
      }
      int atSeenCount = 0;
      for (int i = 1; i < key.length(); i++) {
        char c = key[i];
        if (!isNumberOrDigit(c) && c != '_' && c != '-' && c != '@' && c != '*' && c != '/') {
          return false;
        }
        if ((c == '@') && (++atSeenCount > 1)) {
          return false;
        }
      }
      return true;
    }

  public:
    // Builder class for {@link TraceState}.
    static const class Builder {
      private const TraceState parent;
      private std::vector<Entry> entries; // alternative?

      // Needs to be in this class to avoid initialization deadlock because super class depends on
      // subclass (the auto-value generate class).
      private static const TraceState EMPTY = create(Collections.<Entry>emptyList());

      private Builder(TraceState &parent) {
        this.parent = parent;
        this.entries = null;
      }

      /**
       * Adds or updates the {@code Entry} that has the given {@code key} if it is present. The new
       * {@code Entry} will always be added in the front of the list of entries.
       *
       * @param key the key for the {@code Entry} to be added.
       * @param value the value for the {@code Entry} to be added.
       * @return this.
       * @since 0.1.0
       */
      public Builder set(nostd::string_view key, nostd::string_view value) {
        // Initially create the Entry to validate input.
        Entry entry = Entry.create(key, value);
        if (entries == null) {
          // Copy entries from the parent.
          entries = new ArrayList<>(parent.getEntries());
        }
        for (int i = 0; i < entries.size(); i++) {
          if (entries.get(i).getKey().equals(entry.getKey())) {
            entries.remove(i);
            // Exit now because the entries list cannot contain duplicates.
            break;
          }
        }
        // Inserts the element at the front of this list.
        entries.add(0, entry);
        return this;
      }

      /**
       * Removes the {@code Entry} that has the given {@code key} if it is present.
       *
       * @param key the key for the {@code Entry} to be removed.
       * @return this.
       * @since 0.1.0
       */
      public Builder remove(nostd::string_view key) {
        Utils.checkNotNull(key, "key");
        if (entries == null) {
          // Copy entries from the parent.
          entries = new ArrayList<>(parent.getEntries());
        }
        for (int i = 0; i < entries.size(); i++) {
          if (entries.get(i).getKey().equals(key)) {
            entries.remove(i);
            // Exit now because the entries list cannot contain duplicates.
            break;
          }
        }
        return this;
      }

      /**
       * Builds a TraceState by adding the entries to the parent in front of the key-value pairs list
       * and removing duplicate entries.
       *
       * @return a TraceState with the new entries.
       * @since 0.1.0
       */
      public TraceState build() {
        if (entries == null) {
          return parent;
        }
        return TraceState.create(entries);
      }
    }

    // Immutable key-value pair for TraceState.
    public static class Entry {
      // Creates a new Entry for the TraceState.
      public static Entry create(nostd::string_view &key, nostd::string_view &value) {
        // Again what is this again I am not sure
        return new AutoValue_TraceState_Entry(key, value);
      }

      // Returns the key.
      virtual public nostd::string_view getKey() = 0;

      // Returns the value.
      virtual public nostd::string_view getValue() = 0;

      Entry() {}
    }

    // Returns the default TraceState with no entries.
    static TraceState getDefault() {
      return DEFAULT;
    }

    // Returns the value to which the specified key is mapped, or null if this map contains no mapping
    // for the key.
    nostd::string_view get(nostd::string_view key) {
      for (Entry entry : getEntries()) {
        if (entry.getKey() == key)) {
          return entry.getValue();
        }
      }
      return NULL;
    }

    // Returns a list view of the mappings contained in this TraceState.
    virtual std::vector<Entry> getEntries() = 0;

    // Returns a Builder based on an empty TraceState.
    static Builder builder() {
      return Builder(Builder.EMPTY);
    }

    TraceState() {}
}
}
OPENTELEMETRY_END_NAMESPACE