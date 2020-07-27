// Copyright 2020, OpenTelemetry Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>
#include <map>
#include <iostream>
#include <array>
#include <exception>
#include "opentelemetry/trace/propagation/http_text_format.h"
#include "opentelemetry/trace/span_context.h"
#include "opentelemetry/trace/trace_state.h"
#include "opentelemetry/trace/key_value_iterable.h"
#include "opentelemetry/context/context.h"
#include "opentelemetry/nostd/string_view.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/nostd/variant.h"
//#include "opentelemetry/trace/default_span.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace trace
{
namespace propagation
{
static const nostd::string_view kTraceParent = "traceparent";
static const nostd::string_view kTraceState = "tracestate";
// Parameters no longer needed because the toString functions are resolved else where
static const int kVersionBytes = 2;
static const int kTraceIdBytes = 32;
static const int kParentIdBytes = 16;
static const int kTraceFlagBytes = 2;
static const int kTraceDelimiterBytes = 3;
static const int kHeaderSize = kVersionBytes + kTraceIdBytes + kParentIdBytes + kTraceFlagBytes + kTraceDelimiterBytes;
static const int kTraceStateMaxMembers = 32;
static const nostd::string_view kTraceStateKeyValueDelimiter = "=";
static const int kHeaderElementLengths[4] = {2,32,16,2};
// The HttpTraceContext provides methods to extract and inject
// context into headers of HTTP requests with traces.
// Example:
//    HttpTraceContext.inject(setter,&carrier,&context);
//    HttpTraceContext.extract(getter,&carrier,&context);
template <typename T>
class HttpTraceContext : public HTTPTextFormat<T> {
    public:
        // Rules that manages how context will be extracted from carrier.
        using Getter = nostd::string_view(*)(const T &carrier, nostd::string_view trace_type);

        // Rules that manages how context will be injected to carrier.
        using Setter = void(*)(T &carrier, nostd::string_view trace_type,nostd::string_view trace_description);

        void Inject(Setter setter, T &carrier, const context::Context &context) override {
            trace::SpanContext span_context = GetCurrentSpanContext(context);
            if (!span_context.IsValid()) {
                return;
            }
//            if (span == NULL || !span.GetContext().IsValid()) {
//                // We don't have span.getContext() in span.h, should we just use span? As well as acquiring validity. (I do know how to implement them though)
//                return;
//            }
//            InjectImpl(setter, carrier, span.GetContext());
            InjectImpl(setter, carrier, span_context);
        }

        context::Context Extract(Getter getter, const T &carrier, context::Context &context) override {
            trace::SpanContext span_context = ExtractImpl(getter,carrier);
            nostd::string_view span_key = "current-span";
            if (!span_context.trace_state().empty()) {
                std::cout<<"extract side: non-empty trace state"<<std::endl;
            } else {
                std::cout<<"extract side: empty trace state"<<std::endl;
            }
            nostd::shared_ptr<trace::SpanContext> spc{new trace::SpanContext(span_context)};
            return context.SetValue(span_key,spc);
//            return SetSpanInContext(trace.DefaultSpan(span_context), context);
        }

//        context::Context SetSpanInContext(trace::Span &span, context::Context &context) {
//            nostd::string_view span_key = "current-span";
//            context::Context new_values = context.SetValue(span_key,span);
//            return new_values;
//        }
//
        trace::SpanContext GetCurrentSpanContext(const context::Context &context) {
            const nostd::string_view span_key = "current-span";
            context::Context ctx(context);
            nostd::shared_ptr<trace::SpanContext> span_context = nostd::get<nostd::shared_ptr<trace::SpanContext>>(ctx.GetValue(span_key));
//            if (span_context == nullptr) {
//                return trace::SpanContext();
//            }
            return *(span_context.get());
        }
//        trace::Span GetCurrentSpan(Context &context) {
//            trace::Span span = context.GetValue(Context.kSpanKey);
////            if (span == NULL) {
////                return NULL;
////            }
//            return span;
//        }

    private:

        // TODO: need review on hex_string because trace ids are objects not string_views
//        static void InjectImpl(Setter setter, T &carrier) {
        static void InjectImpl(Setter setter, T &carrier, const trace::SpanContext &span_context) {
            nostd::string_view trace_parent = SpanContextToString(span_context);
            setter(carrier, kTraceParent, trace_parent);
            if (!span_context.trace_state().empty()) {
                std::cout<<"non-empty trace state"<<std::endl;
//                nostd::string_view trace_state = FormatTracestate(span_context.trace_state());
//                setter(carrier, kTraceState, trace_state);
            } else {
                std::cout<<"empty trace state"<<std::endl;
            }
        }

//        static nostd::string_view FormatTracestate(TraceState trace_state) {
//            nostd::string_view res = nostd::string_view("");
//            nostd::span<Entry *> entries = trace_state.entries();
//            int i = 0;
//            for (nostd::span<Entry *>::iterator it = entries.begin(); it != entries.end(); it++) {
//                res += it->key() + "=" + it->value();
//                if (i != entries.size()-1) res += ",";
//            }
////            entries.ForEachKeyValue([&res,&i](nostd::string_view key, nostd::string_view value) { // Is this usage correct?
////                i++;
////                res += key + "=" + value;
////                if (i != entries.size()-1) res += ",";
////            });
//            return res;
//        }

        static nostd::string_view SpanContextToString(const trace::SpanContext &span_context) {
            nostd::span<const uint8_t> trace_id = span_context.trace_id().Id();
            nostd::span<const uint8_t> span_id = span_context.span_id().Id();
            uint8_t trace_flags = span_context.trace_flags().flags();
//            nostd::string_view hex_string = "00-"; // TODO: ask in gitter about string addition
            // Note: This is only temporary replacement for appendable string
            std::string hex_string = "00-";
            for (auto it : trace_id) {
                hex_string += std::string(it,1);
            }
            hex_string += "-";
            for (auto it : span_id) {
                hex_string += std::string(it,1);
            }
            hex_string += "-" + trace_flags;
//            for (auto it : trace_id) {
//                hex_string += nostd::string_view(it,1);
//            }
//            hex_string += "-";
//            for (auto it : span_id) {
//                hex_string += nostd::string_view(it,1);
//            }
//            hex_string += "-";
//            for (auto it : trace_flags) {
//                hex_string += nostd::string_view(it,1);
//            }
            return nostd::string_view(hex_string);
        }

        static trace::SpanContext ExtractContextFromTraceParent(nostd::string_view &trace_parent) {
            bool is_valid = trace_parent.length() == kHeaderSize
                            && trace_parent[kVersionBytes] == '-'
                            && trace_parent[kVersionBytes+kTraceIdBytes+1] == '-'
                            && trace_parent[kVersionBytes+kTraceIdBytes+kParentIdBytes+2] == '-';
            if (!is_valid) {
//                std::cout<<"Unparseable trace_parent header. Returning INVALID span context."<<std::endl;
                return trace::SpanContext();
            }
            try {
                nostd::string_view version;
                nostd::string_view trace_id;
                nostd::string_view span_id;
                nostd::string_view trace_flags;
                int elt_num = 0;
                int countdown = kHeaderElementLengths[elt_num];
                int start_pos = -1;
                for (int i = 0; i < int(trace_parent.size()); i++) {
                    if (trace_parent[i]=='\t') continue;
                    else if (trace_parent[i]=='-') {
                        if (countdown==0) {
                            if (elt_num == 0) {
                                version = trace_parent.substr(start_pos,kHeaderElementLengths[elt_num]);
                            } else if (elt_num == 1) {
                                trace_id = trace_parent.substr(start_pos,kHeaderElementLengths[elt_num]);
                            } else if (elt_num == 2) {
                                span_id = trace_parent.substr(start_pos,kHeaderElementLengths[elt_num]);
                            } else {
                                throw; // Impossible to have more than 4 elements in parent header
                            }
                            countdown = kHeaderElementLengths[++elt_num];
                            start_pos = -1;
                        } else {
                            throw;
                        }
                    } else if ((trace_parent[i]>='a'&&trace_parent[i]<='f')||(trace_parent[i]>='0'&&trace_parent[i]<='9')) {
                        if (start_pos == -1) start_pos = i;
                        countdown--;
                    } else {
                        throw;
                    }
                }
                trace_flags = trace_parent.substr(start_pos,kHeaderElementLengths[elt_num]);

                if (trace_id == "00000000000000000000000000000000" || span_id == "0000000000000000") {
                      return trace::SpanContext();
//                    return SetSpanInContext(trace::DefaultSpan.GetInvalid(), context);
                }
                if (version == "ff") {
                      return trace::SpanContext();
//                    return SetSpanInContext(trace::DefaultSpan.GetInvalid(), context);
                }

                TraceId trace_id_obj = GenerateTraceIdFromString(trace_id);
                SpanId span_id_obj = GenerateSpanIdFromString(span_id);
                TraceFlags trace_flags_obj = GenerateTraceFlagsFromString(trace_flags);
                return trace::SpanContext(trace_id_obj,span_id_obj,trace_flags_obj,TraceState(),true);
//                return trace::SpanContext.CreateFromRemoteParent(trace_id_obj, span_id_obj, trace_flags_obj, TraceState());
            } catch (std::exception& e) {
//                std::cout<<"Unparseable trace_parent header. Returning INVALID span context."<<std::endl;
                return trace::SpanContext();
            }
        }

        static TraceId GenerateTraceIdFromString(nostd::string_view trace_id) {
            const char* tid = trace_id.begin();
            uint8_t buf[16];
            for (int i = 0; i < 32; i++)
            {
                if (i%2==0) {
                    buf[i/2] = (uint8_t)(*tid)*16;
                } else {
                    buf[i/2] += (uint8_t)(*tid);
                }
                tid++;
            }
            return TraceId(buf);
        }

        static SpanId GenerateSpanIdFromString(nostd::string_view span_id) {
            const char* sid = span_id.begin();
            uint8_t buf[8];
            for (int i = 0; i < 16; i++)
            {
                if (i%2==0) {
                    buf[i/2] = (uint8_t)(*sid)*16;
                } else {
                    buf[i/2] += (uint8_t)(*sid);
                }
                sid++;
            }
            return SpanId(buf);
        }

        static TraceFlags GenerateTraceFlagsFromString(nostd::string_view trace_flags) {
            uint8_t buf;
            buf = (uint8_t)(trace_flags[0])*16+(uint8_t)(trace_flags[1]);
            return TraceFlags(buf);
        }
//        static void SetTraceStateBuilder(TraceState.Builder &trace_state_builder, nostd::string_view &list_member) {
//            int index = -1;
//            for (int j = 0; j < list_member.length(); j++) {
//                if (list_member[j] == kTraceStateKeyValueDelimiter) {
//                    index = j;
//                    break;
//                }
//            }
//            if (index == -1) {
//                throw std::invalid_argument("Invalid TraceState list-member format.");
//            }
//            trace_state_builder.Set(list_member.substr(0, index), list_member.substr(index + 1));
//        }

        static TraceState ExtractTraceState(nostd::string_view &trace_state_header) {
         //   TraceState.Builder trace_state_builder = TraceState.builder();
            TraceState trace_state = TraceState();
            int start_pos = -1;
            int end_pos = -1;
            int element_num = 0;
            nostd::string_view list_member;
            for (int i = 0; i < int(trace_state_header.length()); i++) {
                if (trace_state_header[i]=='\t') continue;
                else if (trace_state_header[i]==',') {
                    if (start_pos == -1 && end_pos == -1) continue;
                    element_num++;
                    list_member = trace_state_header.substr(start_pos,end_pos-start_pos+1);
                    AddNewMember(trace_state,list_member);
                    std::cout<<"member added"<<std::endl;
//                    SetTraceStateBuilder(trace_state_builder,list_member); // TODO: work around (std::map? return nullptr?)
                    end_pos = -1;
                    start_pos = -1;
                } else {
                    end_pos = i;
                    if (start_pos==-1) start_pos = i;
                }
            }
            if (start_pos!=-1 && end_pos!=-1) {
                list_member = trace_state_header.substr(start_pos,end_pos-start_pos+1);
                AddNewMember(trace_state,list_member);
//                SetTraceStateBuilder(trace_state_builder,list_member);
                std::cout<<"member added!!!"<<std::endl;
                element_num++;
            }

            if (element_num >= kTraceStateMaxMembers) {
                throw std::invalid_argument("TraceState has too many elements.");
            }
            std::cout<<"trace state returned"<<std::endl;
            return trace_state;
//            return trace_state_builder.Build();
        }

        static void AddNewMember(TraceState &trace_state, nostd::string_view member) {
            for (int i = 0; i < int(member.length()); i++) {
                if (member[i] == '=') {
                    std::cout<<member.substr(0,i)<<" "<<member.substr(i+1,member.length()-i-1)<<std::endl;
                    trace_state.Set(member.substr(0,i),member.substr(i+1,member.length()-i-1));
                    return;
                }
            }
        }

        static trace::SpanContext ExtractImpl(Getter getter, const T &carrier) {
            nostd::string_view trace_parent = getter(carrier, kTraceParent);
            // TODO: ask host why trace_parent's first 8 characters are incorrect
            std::map<std::string,std::string> c = carrier;
            trace_parent = nostd::string_view(c[std::string(kTraceParent)]);
            if (trace_parent == "") {
                return trace::SpanContext();
            }
            trace::SpanContext context_from_parent_header = ExtractContextFromTraceParent(trace_parent);
            if (!context_from_parent_header.IsValid()) {
                return context_from_parent_header;
            }

            nostd::string_view trace_state_header = getter(carrier, kTraceState);
            trace_state_header = nostd::string_view(c[std::string(kTraceState)]);
            if (trace_state_header == "" || trace_state_header.empty()) {
                return context_from_parent_header;
            }

            try {
                TraceState trace_state = ExtractTraceState(trace_state_header);
                std::cout<<"reached here?"<<std::endl;
                return trace::SpanContext(
                    context_from_parent_header.trace_id(),
                    context_from_parent_header.span_id(),
                    context_from_parent_header.trace_flags(),
                    trace_state,
                    true
                );
//                return trace::SpanContext.CreateFromRemoteParent(
//                    context_from_parent_header.GetTraceId(),
//                    context_from_parent_header.GetSpanId(),
//                    context_from_parent_header.GetTraceFlags(),
//                    trace_state
//                );
            } catch (std::exception& e) {
                std::cout<<"exception occured!"<<std::endl;
//                std::cout<<"Unparseable tracestate header. Returning span context without state."<<std::endl;
                return context_from_parent_header;
//                return trace::SpanContext.CreateFromRemoteParent(
//                    context_from_parent_header.GetTraceId(),
//                    context_from_parent_header.GetSpanId(),
//                    context_from_parent_header.GetTraceFlags(),
//                    TraceState.Builder().Build());
            }
        }
};
}
}  // namespace trace
OPENTELEMETRY_END_NAMESPACE
