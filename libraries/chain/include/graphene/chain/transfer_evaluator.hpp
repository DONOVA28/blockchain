/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   class transfer_evaluator: public evaluator<transfer_evaluator>
   {
      public:
         typedef transfer_operation operation_type;

         void_result do_evaluate( const transfer_operation& o );
         void_result do_apply( const transfer_operation& o );

         const asset_dynamic_data_object* asset_dyn_data_ptr = nullptr;
         const account_object*            to_account_ptr = nullptr;

         share_type custom_fee = 0;
   };

   class blind_transfer2_evaluator: public evaluator<blind_transfer2_evaluator>
   {
   public:
      typedef blind_transfer2_operation operation_type;

      void_result do_evaluate( const blind_transfer2_operation& o );
      asset do_apply( const blind_transfer2_operation& o );

      const asset_dynamic_data_object* asset_dyn_data_ptr = nullptr;
      const asset_dynamic_data_object* fee_dyn_data_ptr = nullptr;

      const account_object*            to_account_ptr = nullptr;
      asset custom_fee;
   };

   class update_blind_transfer2_settings_evaluator: public evaluator<update_blind_transfer2_settings_evaluator>
   {
   public:
      typedef update_blind_transfer2_settings_operation operation_type;

      void_result do_evaluate( const update_blind_transfer2_settings_operation& o );
      void_result do_apply( const update_blind_transfer2_settings_operation& o );

   };

   class override_transfer_evaluator: public evaluator<override_transfer_evaluator>
   {
      public:
         typedef override_transfer_operation operation_type;

         void_result do_evaluate( const override_transfer_operation& o );
         void_result do_apply( const override_transfer_operation& o );

         const asset_dynamic_data_object* asset_dyn_data_ptr = nullptr;
         const account_object*            to_account_ptr = nullptr;
   };

} } // graphene::chain
