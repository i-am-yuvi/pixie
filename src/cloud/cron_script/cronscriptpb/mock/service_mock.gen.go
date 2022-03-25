// Code generated by MockGen. DO NOT EDIT.
// Source: service.pb.go

// Package mock_cronscriptpb is a generated GoMock package.
package mock_cronscriptpb

import (
	context "context"
	reflect "reflect"

	gomock "github.com/golang/mock/gomock"
	grpc "google.golang.org/grpc"
	cronscriptpb "px.dev/pixie/src/cloud/cron_script/cronscriptpb"
)

// MockCronScriptServiceClient is a mock of CronScriptServiceClient interface.
type MockCronScriptServiceClient struct {
	ctrl     *gomock.Controller
	recorder *MockCronScriptServiceClientMockRecorder
}

// MockCronScriptServiceClientMockRecorder is the mock recorder for MockCronScriptServiceClient.
type MockCronScriptServiceClientMockRecorder struct {
	mock *MockCronScriptServiceClient
}

// NewMockCronScriptServiceClient creates a new mock instance.
func NewMockCronScriptServiceClient(ctrl *gomock.Controller) *MockCronScriptServiceClient {
	mock := &MockCronScriptServiceClient{ctrl: ctrl}
	mock.recorder = &MockCronScriptServiceClientMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockCronScriptServiceClient) EXPECT() *MockCronScriptServiceClientMockRecorder {
	return m.recorder
}

// CreateScript mocks base method.
func (m *MockCronScriptServiceClient) CreateScript(ctx context.Context, in *cronscriptpb.CreateScriptRequest, opts ...grpc.CallOption) (*cronscriptpb.CreateScriptResponse, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "CreateScript", varargs...)
	ret0, _ := ret[0].(*cronscriptpb.CreateScriptResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateScript indicates an expected call of CreateScript.
func (mr *MockCronScriptServiceClientMockRecorder) CreateScript(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateScript", reflect.TypeOf((*MockCronScriptServiceClient)(nil).CreateScript), varargs...)
}

// DeleteScript mocks base method.
func (m *MockCronScriptServiceClient) DeleteScript(ctx context.Context, in *cronscriptpb.DeleteScriptRequest, opts ...grpc.CallOption) (*cronscriptpb.DeleteScriptResponse, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "DeleteScript", varargs...)
	ret0, _ := ret[0].(*cronscriptpb.DeleteScriptResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// DeleteScript indicates an expected call of DeleteScript.
func (mr *MockCronScriptServiceClientMockRecorder) DeleteScript(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteScript", reflect.TypeOf((*MockCronScriptServiceClient)(nil).DeleteScript), varargs...)
}

// GetScript mocks base method.
func (m *MockCronScriptServiceClient) GetScript(ctx context.Context, in *cronscriptpb.GetScriptRequest, opts ...grpc.CallOption) (*cronscriptpb.GetScriptResponse, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "GetScript", varargs...)
	ret0, _ := ret[0].(*cronscriptpb.GetScriptResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetScript indicates an expected call of GetScript.
func (mr *MockCronScriptServiceClientMockRecorder) GetScript(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetScript", reflect.TypeOf((*MockCronScriptServiceClient)(nil).GetScript), varargs...)
}

// GetScripts mocks base method.
func (m *MockCronScriptServiceClient) GetScripts(ctx context.Context, in *cronscriptpb.GetScriptsRequest, opts ...grpc.CallOption) (*cronscriptpb.GetScriptsResponse, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "GetScripts", varargs...)
	ret0, _ := ret[0].(*cronscriptpb.GetScriptsResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetScripts indicates an expected call of GetScripts.
func (mr *MockCronScriptServiceClientMockRecorder) GetScripts(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetScripts", reflect.TypeOf((*MockCronScriptServiceClient)(nil).GetScripts), varargs...)
}

// UpdateScript mocks base method.
func (m *MockCronScriptServiceClient) UpdateScript(ctx context.Context, in *cronscriptpb.UpdateScriptRequest, opts ...grpc.CallOption) (*cronscriptpb.UpdateScriptRequest, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{ctx, in}
	for _, a := range opts {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "UpdateScript", varargs...)
	ret0, _ := ret[0].(*cronscriptpb.UpdateScriptRequest)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// UpdateScript indicates an expected call of UpdateScript.
func (mr *MockCronScriptServiceClientMockRecorder) UpdateScript(ctx, in interface{}, opts ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{ctx, in}, opts...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "UpdateScript", reflect.TypeOf((*MockCronScriptServiceClient)(nil).UpdateScript), varargs...)
}

// MockCronScriptServiceServer is a mock of CronScriptServiceServer interface.
type MockCronScriptServiceServer struct {
	ctrl     *gomock.Controller
	recorder *MockCronScriptServiceServerMockRecorder
}

// MockCronScriptServiceServerMockRecorder is the mock recorder for MockCronScriptServiceServer.
type MockCronScriptServiceServerMockRecorder struct {
	mock *MockCronScriptServiceServer
}

// NewMockCronScriptServiceServer creates a new mock instance.
func NewMockCronScriptServiceServer(ctrl *gomock.Controller) *MockCronScriptServiceServer {
	mock := &MockCronScriptServiceServer{ctrl: ctrl}
	mock.recorder = &MockCronScriptServiceServerMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockCronScriptServiceServer) EXPECT() *MockCronScriptServiceServerMockRecorder {
	return m.recorder
}

// CreateScript mocks base method.
func (m *MockCronScriptServiceServer) CreateScript(arg0 context.Context, arg1 *cronscriptpb.CreateScriptRequest) (*cronscriptpb.CreateScriptResponse, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateScript", arg0, arg1)
	ret0, _ := ret[0].(*cronscriptpb.CreateScriptResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateScript indicates an expected call of CreateScript.
func (mr *MockCronScriptServiceServerMockRecorder) CreateScript(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateScript", reflect.TypeOf((*MockCronScriptServiceServer)(nil).CreateScript), arg0, arg1)
}

// DeleteScript mocks base method.
func (m *MockCronScriptServiceServer) DeleteScript(arg0 context.Context, arg1 *cronscriptpb.DeleteScriptRequest) (*cronscriptpb.DeleteScriptResponse, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "DeleteScript", arg0, arg1)
	ret0, _ := ret[0].(*cronscriptpb.DeleteScriptResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// DeleteScript indicates an expected call of DeleteScript.
func (mr *MockCronScriptServiceServerMockRecorder) DeleteScript(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteScript", reflect.TypeOf((*MockCronScriptServiceServer)(nil).DeleteScript), arg0, arg1)
}

// GetScript mocks base method.
func (m *MockCronScriptServiceServer) GetScript(arg0 context.Context, arg1 *cronscriptpb.GetScriptRequest) (*cronscriptpb.GetScriptResponse, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetScript", arg0, arg1)
	ret0, _ := ret[0].(*cronscriptpb.GetScriptResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetScript indicates an expected call of GetScript.
func (mr *MockCronScriptServiceServerMockRecorder) GetScript(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetScript", reflect.TypeOf((*MockCronScriptServiceServer)(nil).GetScript), arg0, arg1)
}

// GetScripts mocks base method.
func (m *MockCronScriptServiceServer) GetScripts(arg0 context.Context, arg1 *cronscriptpb.GetScriptsRequest) (*cronscriptpb.GetScriptsResponse, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetScripts", arg0, arg1)
	ret0, _ := ret[0].(*cronscriptpb.GetScriptsResponse)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetScripts indicates an expected call of GetScripts.
func (mr *MockCronScriptServiceServerMockRecorder) GetScripts(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetScripts", reflect.TypeOf((*MockCronScriptServiceServer)(nil).GetScripts), arg0, arg1)
}

// UpdateScript mocks base method.
func (m *MockCronScriptServiceServer) UpdateScript(arg0 context.Context, arg1 *cronscriptpb.UpdateScriptRequest) (*cronscriptpb.UpdateScriptRequest, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "UpdateScript", arg0, arg1)
	ret0, _ := ret[0].(*cronscriptpb.UpdateScriptRequest)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// UpdateScript indicates an expected call of UpdateScript.
func (mr *MockCronScriptServiceServerMockRecorder) UpdateScript(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "UpdateScript", reflect.TypeOf((*MockCronScriptServiceServer)(nil).UpdateScript), arg0, arg1)
}
